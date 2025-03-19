#pragma once
#include <string_view>
#include <ostream>
#include <boost/io/quoted.hpp>
#include <boost/locale/encoding_utf.hpp>
#include "generator.hpp"
#include "util.hpp"

namespace Ru::lexer
{
	/// @brief A _type describing the Token _type
	enum class [[clang::enum_extensibility(closed)]] id : uint_fast8_t
	{
		none,
		skip,

		error,
		error_unclosed_string,
		error_name_unclosed_string,
		error_standalone_quo,
		error_bad_int,

		br_open,     // (
		br_close,    // )
		comma,       // ,
		semicolon,   // ;
		sharp,       // #

		operator_, // +
		op_expl,   // '+
		string,    // "..."
		character, // '_'
		number,    // 0
		identifier,// abc
		id_expl,   // ''abc''
		member,    // .member
		newline,

		//post-lex_raw

		kw__,
		kw_and,
		kw_as,
		kw_by,
		kw_class,
		kw_const,
		kw_else,
		kw_fn,
		kw_for,
		kw_impl,
		kw_in,
		kw_is,
		kw_match,
		kw_module,
		kw_mut,
		kw_not,
		kw_or,
		kw_out,
		kw_priv,
		kw_prp,
		kw_pub,
		kw_return,
		kw_then,
		kw_trait,
		kw_type,
		kw_use,
		kw_with,
		kw_when,
		kw_while,
		kw_yield,

		indent,
		dedent,
		op_init,    // :=
		op_fn,	    // =>
		op_move,    // !
		op_dots,    // ...
		op_exchange,// =
		op_ref,     // &
		op_dot,     // .
		op_either,  // |
		op_pair,    // :

		unit,
		not_in,
	};

	enum class [[clang::enum_extensibility(closed)]] dir : bool
	{
		right, left,
	};

	///\note maybe useless but at least I will leave this as a some form of documentation
	inline consteval uint_fast8_t _precedence(int number, dir direction, dir unary_side)
	{
		return uint_fast8_t(number << 2 | (bool)direction << 1 | (bool)unary_side);
	}

	enum class [[clang::enum_extensibility(closed)]] prec : uint_fast8_t
	{
		unary  =      _precedence(1, dir::left, dir::right),  	///< when the op applies directly after the prev token \example\code f a? b == f (a?) b \endcode\note unary or nullary
		intern =  0u/*_precedence(2, dir::left, dir::right)*/,  ///< (default) \example\code a b ? c == (a b)? c \endcode\note unary or nullary

		pow =         _precedence(3 , dir::right, dir::right), 	///< _**_ \example a ** b \n a **
		mul =         _precedence(4 , dir::left, dir::left),   	///< _*_ _/_ _%_ \example a * b \n *b
		add =         _precedence(5 , dir::left, dir::left),   	///< _+_ _-_

		shift =       _precedence(6 , dir::left, dir::right),  	///< _<<_ _>>_

		bitnot_ =     _precedence(7 , dir::left, dir::left),   	///< _~~_
		bitand_ =     _precedence(8 , dir::left, dir::left),  	///< _&&_
		bitxor_ =     _precedence(9 , dir::left, dir::left),	///< _^^_
		bitor_ =      _precedence(10, dir::left, dir::left),    ///< _||_

		range =       _precedence(11, dir::left, dir::right),   ///< _.._
		cmp =         _precedence(12, dir::right, dir::left),   ///< <= >= < > _!_ _==_ _<>_ \note non-associative

		bidirect =    _precedence(13, dir::left, dir::right),   ///< <:> <-> and so on satisfying both front and back
		front =       _precedence(14, dir::right, dir::left),   ///< _>_
		back =        _precedence(15, dir::left, dir::right),   ///< _<_

		either =      _precedence(16, dir::right, dir::left),  	///< |
		pair =        _precedence(17, dir::right, dir::left),  	///< :
		init =        _precedence(18, dir::left, dir::right),  	///< :=
		comma =       _precedence(19, dir::left, dir::right),   ///< , \note non-associative
		pipe =        _precedence(20, dir::left, dir::right),   ///< _|_

		not_ =        _precedence(21, dir::right, dir::left),   ///< \example not a \note unary-only
		and_ =        _precedence(22, dir::right, dir::left),   ///< \example a and b => c
		or_ =         _precedence(23, dir::right, dir::left),   ///< \example a or b => c
		exchange =    _precedence(24, dir::right, dir::left),   ///< _=_ \example a = b \n a += b \note equal to \c or_ \n binary-only
		while_ =      _precedence(25, dir::right, dir::left),   ///< \example while a \n return a \n fn a => a \note unary-only

		semicolon =   _precedence(26, dir::left, dir::right),   ///< ; endl
		// braces
		inv_open =    (uint_fast8_t)-1, ///< a[ a<[ and so on
		open =        (uint_fast8_t)-2, ///< _[_ {_
		close =       (uint_fast8_t)-3, ///< _]_ _}_

		tree = (uint_fast8_t)-6,        ///< is by prp \example \code
										///<a match is
										///<    1 => 1
										///<    _ => 0
										///< \endcode

		other = (uint_fast8_t)-7,      ///< # =>

	};

	inline auto operator<=>(prec _0, prec _1)
	{
		return (int)_0 <=> (int)_1;
	}

	inline dir associativity(prec prec)
	{
		return (dir)(bool)(std::to_underlying(prec) & 1u << 1);
	}
	inline dir unary_side(prec prec)
	{
		return (dir)(bool)(std::to_underlying(prec) & 1u << 0);
	}

	/// @brief A struct containing a Token metainfo
	struct Token
	{
		/// The Token's _type
		id id = required;
		/// Operator precedence template
		prec prec = prec::intern;
		/// A text Token representation
		std::string_view as_text = required;
		/// line's number from 0
		intptr_t line = required;
		/// utf8-column's number from 0
		intptr_t column = required;
		/// The Token's prefix ('0x'-like thing) size
		intptr_t prefix = 0;
		/// The Token's postfix size
		intptr_t postfix = 0;
		/// That e+123 thing in numbers; quotation marks count for strings
		int64_t shift = 0;


		explicit operator bool() const noexcept {return id != id::none; }
		friend bool operator==(Token const&, Token const&) noexcept = default;
		friend auto& operator<<(std::ostream &out, Token const& self)
		{
			out
				<< "Token{.id = (id)"
				<< (unsigned)self.id
				<< ", .as_text = "
				<< boost::io::quoted((std::string)self.as_text)
			    << ", .line = "
				<< self.line
				<< ", .column = "
				<< self.column;

			if (self.prefix != 0) out
				<< ", .prefix = "
				<< self.prefix;

			if (self.postfix != 0) out
				<< ", .postfix = "
				<< self.prefix;

			if (self.shift != 0) out
				<< ", .shift = "
				<< self.shift;

			return out << "}"
			;
		}
	};

	constexpr inline Token none{
		.id = id::none,
		.as_text = {},
		.line = 0,
		.column = 0,
	};

	constexpr inline Token skip{
		.id = id::skip,
		.as_text = {},
		.line = 0,
		.column = 0,
	};

	using token_generator = generator<Token const&>;

	/// Parses a text into a sequence of tokens
	/// @return A sequence of tokens to parse
	token_generator lex (
	    std::u32string_view input ///< A sequence of lines to lex
	) noexcept;
}
