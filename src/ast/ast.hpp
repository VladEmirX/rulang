#pragma once
#include <utility>
#include "../lexer.hpp"
#include "../util.hpp"

namespace Ru::ast
{
	using lexer::Token;

	struct Expression
	{
		enum class Type
		{
			simple, right, left, apply, binary, braced, left_braced, right_braced, ternary, multiple,
		};

		using ptr_type = std::unique_ptr<Expression>;
		using ptr = _Nonnull ptr_type;
		using mb_ptr = _Nullable ptr_type;

		struct Operator
		{
			mb_ptr left;
			Token token;
		};

		template<class Self, class Fn>
		decltype(auto) visit(this Self&& self, Fn&& fn)
		{
			#define CASE(n) case Type::n: return std::invoke(std::forward<Fn>(fn), static_ref_cast<n>(self))

			switch (self._type)
			{
				CASE(simple);
				CASE(right);
				CASE(left);
				CASE(apply);
				CASE(binary);
				CASE(braced);
				CASE(left_braced);
				CASE(right_braced);
				CASE(ternary);
				CASE(multiple);
			}
			std::unreachable();
			#undef CASE
		}

		template<class Result, class Self, class Fn>
		Result visit(this Self&& self, Fn&& fn)
		{
			#define CASE(n) case Type::n: return std::invoke(std::forward<Fn>(fn), static_ref_cast<n>(self))
			switch (self._type)
			{
				CASE(simple);
				CASE(right);
				CASE(left);
				CASE(apply);
				CASE(binary);
				CASE(braced);
				CASE(left_braced);
				CASE(right_braced);
				CASE(ternary);
				CASE(multiple);
			}
			std::unreachable();
			#undef CASE
		}

		#define CHECK_FN static bool classof(_Nonnull const Expression* expr) {return expr->type == type}

		struct simple : Expression
		{
			static constexpr inline auto type = Type::simple;
			Token token = required;
		};

		struct right : Expression
		{
			static constexpr inline auto type = Type::right;
			ptr left = required;
			Operator op = required;
		};

		struct left : Expression
		{
			static constexpr inline auto type = Type::left;
			Operator op = required;
			ptr right = required;
		};

		struct apply : Expression
		{
			static constexpr inline auto type = Type::apply;
			ptr left = required;
			ptr right = required;
		};

		struct binary : Expression
		{
			static constexpr inline auto type = Type::binary;
			ptr left = required;
			Operator op = required;
			ptr right = required;
		};

		struct braced : Expression
		{
			static constexpr inline auto type = Type::braced;
			Operator open = required;
			ptr mid = required;
			Token close = required;
		};

		struct left_braced : Expression
		{
			static constexpr inline auto type = Type::left_braced;
			Operator open = required;
			ptr mid = required;
			Token close = required;
			ptr right = required;
		};

		struct right_braced : Expression
		{
			static constexpr inline auto type = Type::right_braced;
			ptr left = required;
			Operator open = required;
			ptr mid = required;
			Token close = required;
		};

		struct ternary : Expression
		{
			static constexpr inline auto type = Type::ternary;
			ptr left = required;
			Operator open = required;
			ptr mid = required;
			Token close = required;
			ptr right = required;
		};

		struct multiple : Expression
		{
			static constexpr inline auto type = Type::multiple;
			std::vector<Expression> expressions = required;
		};

	protected: Type _type;
	private: [[msvc::no_unique_address]]
		Empty _type_addr;
	public:
		[[msvc::no_unique_address]]
		RefProperty<&Expression::_type, &Expression::_type_addr> type;

	};

	void i()
	{

	}




}