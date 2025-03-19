#include <boost/test/unit_test.hpp>
#include <array>
#include "../../rulang.h"

using namespace std::string_view_literals;
using namespace Ru::lexer;

BOOST_AUTO_TEST_SUITE(lexer)

BOOST_AUTO_TEST_CASE(strings_in_row)
{
	auto tokens = Ru::lexer::lex(UR"("""abc""""iu""")"sv);
	
	auto tokens_expected = std::array
	{
		Token{id::newline, UR"()"sv},
		Token{id::string, UR"("""abc""")"sv},
		Token{id::string, UR"("iu")"sv},
		Token{id::string, UR"("")"sv},
		Token{id::newline, UR"()"sv},
		Token{id::finale, UR"()"sv},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(tokens.begin(), tokens.end(), tokens_expected.begin(), tokens_expected.end());
}
BOOST_AUTO_TEST_CASE(numbers)
{
	auto tokens = Ru::lexer::lex(UR"(22_222_-22_222      -0.+0   .f 0x.3dp+0 0.0e-x)"sv);
	
	using enum id;
	auto tokens_expected = std::array
	{
		Token{.id = newline, .as_text = U""},
		Token{.id = integer, .as_text = U"22_222_"},
		Token{.id = operator_, .as_text = U"-" },
		Token{.id = integer, .as_text = U"22_222"},
		Token{.id = operator_, .as_text = U"-" },
		Token{.id = floating, .as_text = U"0."},
		Token{.id = operator_, .as_text = U"+"},
		Token{.id = integer, .as_text = U"0"},
		Token{.id = op_dot, .as_text = U"."},
		Token{.id = identifier, .as_text = U"f"},
		Token{.id = floating, .as_text = U"0x.3dp+0"},
		Token{.id = floating, .as_text = U"0.0e"},
		Token{.id = operator_, .as_text = U"-"},
		Token{.id = identifier, .as_text = U"x"},
		Token{.id = newline, .as_text = U""},
		Token{.id = finale, .as_text = U""},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(tokens.begin(), tokens.end(), tokens_expected.begin(), tokens_expected.end());
}
BOOST_AUTO_TEST_CASE(multiline)
{
	auto tokens = Ru::lexer::lex(U"( ///abc\r    qwerty\n     :=\r\n//kuk\n\n\n. !. .!."sv);


	using enum id;
	auto expected = std::array
	{
		Token{.id = newline, .as_text = U""},
		Token{.id = br_open, .as_text = U"("},
		Token{.id = indent, .as_text = U"    "},
		Token{.id = identifier, .as_text = U"qwerty"},
		//Token{.id = newline, .as_text = U"     "},
		Token{.id = op_acq, .as_text = U":="},
		Token{.id = dedent, .as_text = U""},
		Token{.id = newline, .as_text = U""},
		Token{.id = op_dot, .as_text = U"."},
		Token{.id = op_move, .as_text = U"!"},
		Token{.id = op_dot, .as_text = U"."},
		Token{.id = operator_, .as_text = U".!."},
		Token{.id = newline, .as_text = U""},
		Token{.id = finale, .as_text = U""},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(tokens.begin(), tokens.end(), expected.begin(), expected.end());

}

BOOST_AUTO_TEST_CASE(quotes)
{
	auto tokens = Ru::lexer::lex(UR"(   '1'2'a+-''123''''' ''''1234543215'''''{/}english or spanish... '... //english or spanish...)"sv);
	
	using enum id;
	auto expected = std::array
	{
		//Token{.id = newline, .as_text = U"   "},
		Token{.id = character, .as_text = U"1"},
		Token{.id = integer, .as_text = U"2"},
		Token{.id = op_expl, .as_text = U"a+-"},
		Token{.id = id_expl, .as_text = U"123"},
		Token{.id = character, .as_text = U"'"},
		Token{.id = id_expl, .as_text = U"1234543215"},
		Token{.id = error_standalone_quo, .as_text = U"'"},
		Token{.id = br_cur_open, .as_text = U"{"},
		Token{.id = operator_, .as_text = U"/" },
		Token{.id = br_cur_close, .as_text = U"}"},
		Token{.id = identifier, .as_text = U"english"},
		Token{.id = kw_or, .as_text = U"or" },
		Token{.id = identifier, .as_text = U"spanish"},
		Token{.id = op_dots, .as_text = U"..."},
		Token{.id = op_expl, .as_text = U"..."},
		Token{.id = newline, .as_text = U""},
		Token{.id = finale, .as_text = U""},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(tokens.begin(), tokens.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()