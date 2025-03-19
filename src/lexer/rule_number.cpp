#include <locale>
#include <boost/spirit/include/qi.hpp>
#include "../lexer.hpp"

namespace Ru::lexer::lex_raw
{
	Ru::lexer::Token subrule_number(
		char const*& __restrict begin,
		char const* const end,
		intptr_t& __restrict line,
		char const*& __restrict line_start
	) noexcept
	{
		namespace qi = boost::spirit::qi;

		char const* const orig_begin = begin;

		Token result = {.id = id::number, .line = line, .column = begin - line_start};

		auto hex_prefix = (qi::lit("0x") | "0X" )[([&] () {result.prefix = 2;})];
		auto bin_prefix = (qi::lit("0b") | "0B" )[([&] () {result.prefix = 2;})];

		auto digits = +qi::digit % '\'';
		auto xdigits = +qi::xdigit % '\'';
		auto bdigits = +(qi::lit('0') | '1') % '\'';

		auto expr =
			(
				-(-digits >> '.') >> digits >> -(
				    (qi::lit('e') | 'E')
					>> qi::long_long[([&] (int64_t value) {result.shift = value;})]
			    )
			)
			| (hex_prefix
			   >> -(-xdigits >> '.') >> xdigits >> -(
			   (qi::char_('p') | qi::char_('P'))
			       >> qi::long_long[([&] (int64_t value) {result.shift = value;})]
				)
			)
			| (bin_prefix
			   >> -(-bdigits >> '.') >> bdigits >> -(
			   (qi::char_('p') | qi::char_('P'))
			       >> qi::long_long[([&] (int64_t value) {result.shift = value;})]
				)
			)
			;

		auto ok = qi::parse(begin, end, expr);
		if (not ok or orig_begin == begin) return none;

		result.as_text = std::string_view(orig_begin, begin);
		return result;
	}
}