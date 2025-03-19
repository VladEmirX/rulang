#include <unordered_set>
#include "../lexer.hpp"

namespace Ru::lexer
{
	static token_generator apply_none(token_generator tokens) noexcept
	{
		for (auto const& tok : tokens)
		{
			co_yield tok;
		}
		co_yield none;
	}


	/// Performs a primary splitting the text
	extern token_generator lex_raw(std::string_view input) noexcept;

	/// Detects the keywords and changes the ids of the keyword identifiers tokens
	static token_generator identifiers(token_generator tokens) noexcept
	{
		for (auto const& tok: tokens)
		{
			if (tok.id != id::identifier)
			{
				co_yield tok;
				continue;
			}

			using namespace std::string_view_literals;
			#define kw(word, precedence) { #word ## sv , { id::kw_ ## word, prec::precedence } }
			static auto const keywords = std::unordered_map<std::string_view, std::pair<id, prec>>
			{
				kw(in, cmp),
				kw(out, intern),
				kw(mut, intern),
				kw(const, intern),
				kw(return, while_),
				kw(yield, while_),
				kw(type, while_),
				kw(trait, while_),
				kw(class, while_),
				kw(fn, while_),
				kw(module, while_),
				kw(impl, intern),
				kw(use, intern),
				kw(with, intern),
				kw(when, intern),
				kw(as, intern),
				kw(not, not_),
				kw(then, and_),
				kw(else, or_),
				kw(and, and_),
				kw(or, or_),
				kw(for, and_),
				kw(while, while_),
				kw(_, intern),
				kw(priv, intern),
				kw(pub, intern),
				kw(match, and_),
				kw(is, tree),
				kw(by, tree),
				kw(prp, tree),
			};
			#undef kw

			auto const found = keywords.find(tok.as_text);
			if (keywords.end() == found) co_yield tok;
			else
			{
				auto token_copy = tok;
				token_copy.id = found->second.first;
				token_copy.prec = found->second.second;
				co_yield token_copy;
			}
		}
	}

	/// Detects operator tokens that end with '.' and split them into two
	static token_generator dot_at_right(token_generator tokens) noexcept
	{
		for (auto const& tok: tokens)
		{
			if (tok.id != id::operator_
			or tok.as_text.back() != '.'
			or tok.as_text.size() == 1u
			or tok.as_text.end()[-2] == '.')
			{
				co_yield tok;
				continue;
			}

			auto token_copy = tok;
			token_copy.as_text = tok.as_text.substr(0u, tok.as_text.size() - 1u);
			co_yield token_copy;
			co_yield Token{
				.id = id::op_dot,
				.as_text = {tok.as_text.end() - 1, tok.as_text.end()},
				.line = tok.line,
				.column = tok.column - 1 + intptr_t(tok.as_text.length()),
			};
		}
	}

	/// Detects operator tokens that begin with '.' and split them into two
	static token_generator dot_at_left(token_generator tokens) noexcept
	{
		for (auto const& tok: tokens)
		{
			if (tok.id != id::operator_
			or tok.as_text.front() != '.'
			or tok.as_text.size() == 1u
			or tok.as_text.end()[1] == '.')
			{
				co_yield tok;
				continue;
			}

			auto token_copy = tok;
			token_copy.as_text = tok.as_text.substr(1u, tok.as_text.size());
			token_copy.column++;
			co_yield Token{
				.id = id::op_dot,
				.as_text = tok.as_text.substr(0u, 1u),
				.line = tok.line,
				.column = tok.column,
			};
			co_yield token_copy;
		}
	}

	/// Detects the operators keywords and changes the ids of the keyword operators tokens
	static token_generator operators(token_generator tokens) noexcept
	{
		for (auto&& tok: tokens)
		{
			if (tok.id != id::operator_)
			{
				co_yield tok;
				continue;
			}

			using namespace std::string_view_literals;
			#define kwop(op, name, precedence) { #op ## sv, {id::op_ ## name, prec::precedence}}
			static auto const keywords = std::unordered_map<std::string_view, std::pair<id, prec>>
			{
				kwop(:= , init, other),
				kwop(=> , fn, other),
				kwop( ! , move, intern),
				kwop(..., dots, intern),
				kwop( = , exchange, exchange),
				kwop( & , ref, intern),
				kwop( . , dot, intern),
				kwop( | , either, either),
				kwop( : , pair, pair),
			};
			#undef kwop

			auto token_copy = tok;

			if (auto const at = keywords.find(tok.as_text);
			    at != keywords.end()
				)
			{
				token_copy.id = at->second.first;
				token_copy.prec = at->second.second;
			}

			co_yield token_copy;
		}
	}

	static token_generator noexpl(token_generator tokens) noexcept
	{
		for (auto const& tok: tokens)
		{
			if (tok.id == id::id_expl)
			{
				auto copy = tok;
				copy.id = id::identifier;
				co_yield copy;
				continue;
			}
			if (tok.id == id::op_expl)
			{
				auto copy = tok;
				copy.id = id::operator_;
				co_yield copy;
				continue;
			}
			co_yield tok;
		}
	}

	constexpr static auto mix(id left, id right, id result)
	{
		return [=] (token_generator tokens) noexcept -> token_generator
		{
			Token curr = none, prev = none;
			for (auto const& tok : std::move(tokens) ->* apply_none)
			{
				prev = curr;
				curr = tok;

				if (prev.id == left
				    and curr.id == right
				    and curr.as_text.data() == prev.as_text.data() + prev.as_text.size())
				{
					auto copy = curr;
					copy.id = result;
					copy.as_text = {prev.as_text.begin(), curr.as_text.end()};
					copy.column = prev.column;
					copy.prefix = prev.as_text.size();
					co_yield curr;
				}
				else co_yield prev;
			}
		};
	}

	extern token_generator precedence(token_generator tokens) noexcept;

	/// Detects the indentation changes and generates new indent/dedent tokens based on the input
	static token_generator indents(token_generator tokens) noexcept
	{
        auto prev = none, curr = none;
        auto indents = std::vector<size_t>{0uz};
		for (auto const& tok : tokens)
		{
			prev = curr;
			curr = tok;
			if (tok.id != id::newline)
			{
				co_yield tok;
				continue;
			}

			if (tok.as_text.size() > indents.back()) switch (prev.prec)
            {
	            default: break;
				case prec::open:
				case prec::inv_open:
				case prec::and_:
				case prec::or_:
				case prec::while_:
				case prec::exchange:
				case prec::other:
					auto token_copy = tok;
					token_copy.id = id::indent;
					token_copy.prec = prec::open;
					co_yield token_copy;
					indents.push_back(tok.as_text.size());
					break;
            }
			else
			{
				auto dedent_tok = tok;
				dedent_tok.prec = prec::close;
				dedent_tok.id = id::dedent;
				while (tok.as_text.size() < indents.back())
				{
					co_yield dedent_tok;
					indents.pop_back();
				}

				if (tok.as_text.size() == indents.back())
				{
					co_yield tok;
				}
			}
		}
	}

	static token_generator invoke(token_generator tokens) noexcept
	{
		auto prev = none, curr = none;
		for (auto const& tok : tokens)
		{
			prev = curr;
			curr = tok;

			if (prev.id != id::op_dot
			and (prev.prec == prec::close or prev.prec == prec::intern or prev.prec == prec::unary)
			and prev.as_text.data() + prev.as_text.size() == curr.as_text.data())
			{
				if (curr.prec == prec::open)
					curr.prec = prec::inv_open;
				if (curr.prec == prec::intern)
					curr.prec = prec::unary;
			}

			co_yield curr;
		}
	}

	token_generator lex(std::string_view input) noexcept
	{
		return input
			->* lex_raw
			->* identifiers
			->* dot_at_right
			->* dot_at_left
			->* operators
			->* mix(id::op_move, id::kw_in, id::not_in)
			->* precedence
			->* indents
			->* noexpl
			->* invoke
	    ;
	}
}