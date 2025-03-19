#include <boost/spirit/include/qi.hpp>
#include <ranges>
#include "parser.hpp"

namespace Ru::parse
{
	namespace ast = Ru::ast;
	namespace spt = boost::spirit;
	namespace qi = boost::spirit::qi;
	using namespace Ru::lexer;

	struct TokenWrapper;

	using Ru::Shared;
	using vec = std::vector<Token>;
	using iter = vec::const_iterator;
	using unused = qi::unused_type;
	using rule = qi::rule<iter, Shared<ast::Expression>()>;
	using token_rule = qi::rule<iter, TokenWrapper()>;

	struct TokenWrapper
	{
		Token token;
		explicit(false) TokenWrapper(Token const& token = none) noexcept
		: token(token)
		{}
		auto& operator=(Token _token) noexcept
		{
			token = _token;
			return *this;
		}
		explicit(false) operator Token() const noexcept
		{
			return token;
		}
	};

	struct token_t : boost::spirit::qi::primitive_parser<token_t>
	{
		template<class Iter>
		bool parse(Iter& begin, Iter const& end, unused, unused, TokenWrapper& token) const
		{
			if (begin == end) return false;
			if (prec and begin->prec != prec) return false;
			if (id != id::none and begin->id != id) return false;

			token = *begin;

			++begin;
			return true;
		}

		auto what(auto const&) const
		{
			return qi::info("token", "");
		}

		template<class Context, class Iterator>
		struct attribute
		{
			using type = TokenWrapper;
		};

		std::optional<prec> prec = std::nullopt;
		id id = id::none;
	};


	struct SimpleRule : qi::grammar<vec::const_iterator, Shared<ast::Expression>()>
	{
		explicit SimpleRule(std::optional<prec> prec = std::nullopt) :base_type(rule)
		{
			simple = token_t{.prec = prec};
			rule = simple[([](auto const& t){
					return Shared<ast::SimpleExpression>::from(t);
				})];
		}
	private:
		token_rule simple;
		rule rule;
	};

	struct DottedRule : qi::grammar<vec::const_iterator, Shared<ast::Expression>()>
	{
		explicit DottedRule(rule& left, prec dot_prec, prec prec) : base_type(rule)
		{
			dot = token_t{.prec = dot_prec, .id = id::op_dot};
			right = token_t{.prec = prec};
			rule = (left >> dot >> right)[([](auto const& t){
				return Shared<ast::DotExpression>::from(at_c<0>(t), at_c<1>(t), at_c<2>(t));
			})];
		}
	private:
		token_rule dot, right;
		rule rule;
	};

	struct Parser : qi::grammar<vec::const_iterator, Shared<ast::Expression>()>
	{
		Parser() : base_type(everything_rule)
		{
			using boost::fusion::at_c;

			auto unary_precedence =
				[this] (prec prec) {
					return DottedRule(unary_rule, prec::unary, prec) | SimpleRule(prec);};
			auto precedence =
				[this] (prec prec) {
					return DottedRule(intern_rule, prec::intern, prec) | DottedRule(unary_rule, prec::unary, prec) | SimpleRule(prec);};

			unary_braced_rule = (SimpleRule(prec::inv_open) >> everything_rule >> token_t{.prec = prec::close})[([](auto const& t){
				return Shared<ast::BracedExpression>::from(at_c<0>(t), at_c<1>(t), at_c<2>(t));
			})] | (SimpleRule(prec::inv_open) >> token_t{.prec = prec::close})[([](auto const& t){
				return Shared<ast::EmptyBracedExpression>::from(at_c<0>(t), at_c<1>(t));
			})];
			unary_invoke_rule = (unary_rule >> (unary_braced_rule | SimpleRule(prec::unary)))[([](auto const& t){
				return Shared<ast::UnaryExpression>::from(at_c<0>(t), at_c<1>(t));
			})];

			braced_rule = (unary_precedence(prec::open) >> everything_rule >> token_t{.prec = prec::close})[([](auto const& t){
				return Shared<ast::BracedExpression>::from(at_c<0>(t), at_c<1>(t), at_c<2>(t));
			})]| (unary_precedence(prec::open) >> token_t{.prec = prec::close})[([](auto const& t){
				return Shared<ast::EmptyBracedExpression>::from(at_c<0>(t), at_c<1>(t));
			})];
			invoke_rule = (intern_rule >> unary_rule)[([](auto const& t){
				return Shared<ast::UnaryExpression>::from(at_c<0>(t), at_c<1>(t));
			})];

			unary_rule = unary_precedence(prec::intern) | unary_invoke_rule | braced_rule;

			intern_rule = DottedRule(intern_rule, prec::intern, prec::intern) | invoke_rule | unary_rule;

			auto left =
				[&] (prec prec, rule& right) {
					return (precedence(prec) >> right)[([](auto const& t){
						return Shared<ast::UnaryExpression>::from(at_c<0>(t), at_c<1>(t));
					})];};
			auto right =
				[&] (rule& left, prec prec) {
					return (left >> precedence(prec))[([](auto const& t){
						return Shared<ast::UnaryExpression>::from(at_c<0>(t), at_c<1>(t));
					})];};
			auto binary =
				[&] (rule& left, prec prec, rule& right) {
					return (left >> precedence(prec) >> right)[([](auto const& t){
						return Shared<ast::BinaryExpression>::from(at_c<0>(t), at_c<1>(t), at_c<2>(t));
					})];};


			rule* prev = &intern_rule;

			for ( auto&& [prec, curr]: std::views::zip(precs, precs_rules))
			{
				if (unary_side(prec) == dir::left)
					curr = left(prec, curr);
				else curr = right(curr, prec);

				if (associativity(prec) == dir::left)
					curr = binary(curr, prec, *prev) | curr;
				else curr = binary(*prev, prec, curr) | curr;

				curr = curr | *prev;
				prev = &curr;
			}

			rule& pipe_rule = *prev;

			not_rule = left(prec::not_, not_rule) | pipe_rule;






		}

	private:

		static constexpr inline prec precs[]{
			prec::pow, prec::mul, prec::add, prec::shift, prec::bitnot_, prec::bitand_,
			prec::bitxor_, prec::bitor_, prec::range, prec::cmp, prec::bidirect, prec::front,
			prec::back, prec::either, prec::pair, prec::init, prec::comma, prec::pipe
		};

		rule
			unary_braced_rule,
			unary_invoke_rule,
			braced_rule,
			invoke_rule,
			unary_rule,
			intern_rule,
			precs_rules[std::size(precs)],
			not_rule,
			and_rule,
			or_rule,
			exch_rule,
			while_rule,
			fn_rule,
			while_fn_rule = while_rule | fn_rule | exch_rule,
			everything_rule;



	};



	Ru::ast::ExpressionPtr parse(token_generator tokens_raw)
	{
		auto tokens = vec();
		for (auto tok: tokens_raw)
			tokens.push_back(tok);

		Ru::ast::ExpressionPtr result;

		bool is_ok = qi::parse(tokens.begin(), tokens.end(), Parser{}, result);

		return result;
	}
}



