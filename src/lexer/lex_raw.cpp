#include <locale>
#include <optional>
#include "../lexer.hpp"


namespace ru::lexer::lex_raw
{
	static std::pair<char32_t, size_t> to_utf32(char const* begin, char const* end)
	{
		auto original_begin = begin;
		auto result = boost::locale::utf::utf_traits<char>::decode(begin, end);

		if (result == boost::locale::utf::illegal || result == boost::locale::utf::incomplete)
			throw boost::locale::conv::conversion_error();

		return {(char32_t)result, begin - original_begin};
	}

	static bool ctype_is(std::ctype_base::mask m, char const* begin, char const* end, size_t* len = nullptr)
	{
		if (begin == end) return false;
		if ((signed char)*begin >= 0)
		{
			if (len != nullptr) *len = 1u;
			if (*begin == '_' and m & std::ctype_base::lower)
				return true;
			switch (*begin)
			{
				case '_': case '"': case '\'': case ',': case ';': case '#': case '(': case ')':
					m &= ~std::ctype_base::punct;
				default:
			}
			return std::use_facet<std::ctype<char>>(std::locale()).is(m, *begin);
		}
		auto [ch, _len] = to_utf32(begin, end);
		if (len != nullptr) *len = _len;
		return std::use_facet<std::ctype<char32_t>>(std::locale()).is(m, ch);
	}

	static const char* ctype_scan_not(std::ctype_base::mask m, char const* begin, char const* end)
	{
		size_t len;
		while (ctype_is(m, begin, end, &len)) begin += len;
		return begin;
	}

	using Ru::lexer::id;
	using Ru::lexer::prec;
	using Ru::lexer::none;
	using Ru::lexer::skip;

	template <id id, prec prec, char ch>
	static Ru::lexer::Token rule_symb(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
		) noexcept
	{
		if (begin != end); else return none;
		if (*begin == ch); else return none;
		++begin;
		return { .id = id, .prec = prec, .as_text = {begin - 1, begin}, .line = line, .column = begin - 1 - line_start };
	}


	static Ru::lexer::Token rule_comment(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		if (begin + 1 < end); else return none;
		if (begin[0] == '#' and begin[1] == '#'); else return none;

		while (begin != end and *begin != '\n' and *begin != '\r') ++begin;
		return skip;
	}

	static void subrule_skip_endl(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		bool moved = false;
		if (begin != end and *begin == '\r') { ++begin; moved = true; }
		if (begin != end and *begin == '\n') { ++begin; moved = true; }
		if (moved) {++line; line_start = begin;}
	}

	static Ru::lexer::Token subrule_indent(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		auto const token_begin = begin;
		while (begin != end and *begin == ' ') ++begin;
		return { .id = id::newline, .prec = prec::semicolon, .as_text {token_begin, begin}, .line = line, .column = token_begin - line_start};
	}

	static Ru::lexer::Token subrule_newline(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		for (;;)
		{
			subrule_skip_endl(begin, end, line, line_start);
			auto token = subrule_indent(begin, end, line, line_start);
			rule_comment(begin, end, line, line_start);
			if (begin != end); else return none;
			if (*begin == '\r' or *begin == '\n'); else return token;
		}
	}

	static Ru::lexer::Token rule_newline(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		if (begin != end); else return none;
		if (*begin == '\r' or *begin == '\n'); else return none;

		return subrule_newline(begin, end, line, line_start);
	}

	Ru::lexer::Token rule_number(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept;

	static Ru::lexer::Token rule_string(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		if (begin != end); else return none;
		if (*begin == '"'); else return none;
		
		auto const token_begin = begin;

		while ( begin != end and *begin == '"') ++begin;
		auto const quo_open = std::string_view(token_begin, begin);
		auto const length = quo_open.length();

		if (length != 2u); else return {.id = id::string, .as_text = quo_open, .line = line, .column = token_begin - line_start};

		auto const is_not_end = [&begin, length, quo_open]
		{ 
			return std::string_view(begin - length, begin) != quo_open;
		};

		auto const fst_line_start = line_start;

		++begin;
		while (begin != end and is_not_end()) { ++begin; subrule_skip_endl(begin, end, line, line_start); }

		return {
			.id = is_not_end() ? id::error_unclosed_string : id::string,
			.as_text {token_begin, begin},
			.line = line,
			.column = token_begin - fst_line_start
		};
	}

	static Ru::lexer::Token rule_name(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		if (begin != end); else return none;

		if (ctype_is(std::ctype_base::alpha, begin, end)); else return none;

		auto const token_begin = begin;
		begin = ctype_scan_not(std::ctype_base::alnum, begin, end);

		return {.id = id::identifier, .as_text {token_begin, begin}, .line = line, .column = token_begin - line_start};
	}

	static Ru::lexer::Token subrule_char(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		if (begin != end); else return { .id = id::error_standalone_quo, .as_text {begin - 1, begin}, .line = line, .column = begin - 1 - line_start };

		if (*begin == '\\')
		{
			auto const token_begin = begin;
			++begin;

			if (begin != end); else return { .id = id::error_unclosed_string, .as_text {token_begin - 1, begin}, .line = line, .column = token_begin - 1 - line_start };

			++begin;

			if (begin != end); else return { .id = id::error_unclosed_string, .as_text {token_begin - 1, begin}, .line = line, .column = token_begin - 1 - line_start };

			switch (begin[-1])
			{
				case 'x':
				case 'X':
				case 'u':
				case 'U':
					begin = ctype_scan_not(std::ctype_base::xdigit, begin, end);
				default:
			}

			if (begin != end and *begin == '\''); else return { .id = id::error_unclosed_string, .as_text {token_begin - 1, begin}, .line = line, .column = token_begin - 1 - line_start};
			
			++begin;
			return { .id = id::character, .as_text {token_begin, begin - 1}, .line = line, .column = token_begin - line_start };
		}
		else if (begin + 1 != end and begin[1] == '\'') { begin += 2; return {
			.id = id::character,
			.as_text {begin - 2, begin - 1},
			.line = line,
			.column = begin - 2 - line_start
		}; }
		else
		{
			auto const token_begin = begin;
			begin = ctype_scan_not(std::ctype_base::graph, begin, end);
			if (begin != token_begin)
				return { .id = id::op_expl, .as_text {token_begin, begin}, .line = line, .column = token_begin - line_start };
			else return { .id = id::error_standalone_quo, .as_text {begin - 1, begin}, .line = line, .column = begin - 1 - line_start};
		}
	}

	static Ru::lexer::Token rule_char(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		if (begin != end); else return none;
		if (*begin == '\''); else return none;

		auto const token_begin = begin;

		for (; begin != end and *begin == '\''; ++begin);
		auto const quo_open = std::string_view(token_begin, begin);
		auto const length = quo_open.length();

		if (length != 1u); else return subrule_char(begin, end, line, line_start);
		if (length != 3u); else return { .id = id::character, .as_text {token_begin + 1, token_begin + 2}, .line = line, .column = token_begin + 1 - line_start};

		auto const is_not_end = [&begin, length, quo_open]
		{
			return
				std::string_view(begin - length, begin) != quo_open
				//or *begin == '\''
				;
		};

		++begin;
		while (begin != end and *begin != '\r' and *begin != '\n' and is_not_end()) ++begin;

		return {
			.id = is_not_end() ? id::error_name_unclosed_string : id::id_expl,
			.as_text = {token_begin + length, begin - length},
			.line = line,
			.column = token_begin + length - line_start,
			.shift = (int64_t)length,
		};
	}

	static Ru::lexer::Token rule_operator(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		if (begin != end); else return none;
		if (ctype_is(std::ctype_base::punct, begin, end)); else return none;

		auto const token_begin = begin;

		begin = ctype_scan_not(std::ctype_base::punct, begin, end);
		return { .id = id::operator_, .as_text {token_begin, begin}, .line = line, .column = token_begin - line_start};
	}
	static Ru::lexer::Token rule_error(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		++begin;
		return {.id = id::error, .as_text {begin - 1, begin}, .line = line, .column = begin - 1 - line_start};
	}

	constexpr auto rules = std::array
	{
		&rule_symb<id::skip, prec::other, ' '>,
		&rule_symb<id::br_open, prec::open, '('>,
		&rule_symb<id::br_close, prec::close, ')'>,
		&rule_symb<id::comma, prec::comma, ','>,
		&rule_symb<id::semicolon, prec::semicolon, ';'>,
		&rule_symb<id::sharp, prec::other, '#'>,

		&rule_comment,
		&rule_newline,
		&rule_number,
		&rule_string,
		&rule_name,
		&rule_char,
		&rule_operator,
		&rule_error,
	};
}

namespace Ru::lexer
{

	token_generator lex_raw(std::string_view input) noexcept
	{
		using namespace ru::lexer::lex_raw;

		auto begin = input.data();
		auto const end = begin + input.length();
		auto line = 0z;
		auto line_begin = begin;

		co_yield subrule_newline(begin, end, line, line_begin);

		while (begin < end)
			for (auto const rule : rules)
				if (auto result = rule(begin, end, line, line_begin))
				{
					if (result.id != id::skip) co_yield result;
					break;
				}
		co_yield subrule_newline(begin, end, line, line_begin);
	}
}