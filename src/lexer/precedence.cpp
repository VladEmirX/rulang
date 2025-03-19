#include <unordered_map>
#include "../lexer.hpp"

namespace Ru::lexer
{
	static const auto symb_infixes = std::unordered_map<char, prec>
		{
			{'!', prec::cmp},
			{'=', prec::exchange},
			{'<', prec::back},
			{'>', prec::front},
			{'|', prec::pipe},
			{'[', prec::open},
			{'{', prec::open},
			{']', prec::close},
			{'}', prec::close},
			{'*', prec::mul},
			{'/', prec::mul},
			{'%', prec::mul},
			{'+', prec::add},
			{'-', prec::add},
		};

	static const auto two_symb_infixes = std::unordered_map<char, prec>
		{
			{'=', prec::cmp},
			{'<', prec::shift},
			{'>', prec::shift},
			{'*', prec::pow},
			{'~', prec::bitnot_},
			{'&', prec::bitand_},
			{'^', prec::bitxor_},
			{'|', prec::bitor_},
			{'.', prec::range},
		};

	static bool is_cmp(std::string_view input)
	{
		if (input == "<") return true;
		if (input == ">") return true;
		if (input == "<=") return true;
		if (input == ">=") return true;

		return false;
	}

	static prec get_precision(std::string_view input)
	{
		if (is_cmp(input)) return prec::cmp;

		char prev = 0, curr = 0;
		bool is_open = false, is_close = false, is_back = false, is_front = false;
		prec max_un_prec = prec::intern, max_bin_prec = prec::intern;
		for (char const ch: input)
		{
			prev = curr;
			curr = ch;

			if (prev == curr)
			{
				auto found_bin_it = two_symb_infixes.find(curr);
				if (found_bin_it != two_symb_infixes.end())
					max_bin_prec = std::max(max_bin_prec, found_bin_it->second);
			}
			if (prev == '<' and curr == '>')
				max_bin_prec = std::max(max_bin_prec, prec::cmp);

			auto found_un_it = symb_infixes.find(curr);
			if (found_un_it != two_symb_infixes.end())
			{
				prec found_un = found_un_it->second;
				max_un_prec = std::max(max_un_prec, found_un);
				switch (found_un)
				{
					default:
					break; case prec::open: is_open = true;
					break; case prec::close: is_close = true;
					break; case prec::front: is_front = true;
					break; case prec::back: is_back = true;
					break; case prec::cmp: max_bin_prec = std::max(max_un_prec, prec::cmp);

				}
			}
		}

		if (is_open and is_close) return prec::intern;
		if (is_open or is_close
		or max_un_prec == prec::pipe and max_bin_prec != prec::bitor_
		or max_un_prec == prec::exchange and max_bin_prec != prec::cmp and max_bin_prec != prec::range)
			return max_un_prec;
		if (max_bin_prec != prec::intern) return max_bin_prec;
		if (is_front and is_back) return prec::bidirect;
		return max_un_prec;
	};

	extern token_generator precedence(token_generator tokens) noexcept
	{
		for (auto const& tok: tokens)
		{
			if (tok.id == id::operator_)
			{
				Token copy = tok;
				copy.prec = get_precision(tok.as_text);
			}
			else co_yield tok;
		}
	}
}