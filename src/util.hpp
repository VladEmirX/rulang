#pragma once
#include <utility>
#include <concepts>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <boost/container_hash/hash.hpp>

namespace Ru
{
	/**
	 * @brief A convenient pipe operator
	 * @tparam selfT An argument _type
	 * @tparam fnT A function _type
	 * @param self A function argument
	 * @param fn A function to apply
	 * @return Result of completion of @c fn(self)
	 *
	 * A simple one-argument pipe operator that may be used in way
	 * @code
	 * arg
	 * ->* fn0
	 * ->* fn1
	 * ->* ...
	 * @endcode
	 * , which will be equivalent to
	 * @code
	 * ...(fn1(fn0(arg)))
	 * @endcode
	 */
	template<class selfT, class fnT>
	[[clang::always_inline]]
	inline constexpr decltype(auto) operator->*(selfT &&self, fnT &&fn) noexcept(noexcept(
		std::forward<fnT>(fn)(std::forward<selfT>(self))))
	{
		return std::forward<fnT>(fn)(std::forward<selfT>(self));
	}

	struct Required
	{
		template<class T>
		explicit(false) operator T() const noexcept
		{
			static_assert(false, "The field is uninitialized");
		}
		constexpr Required() noexcept = default;
		~Required() = default;
		Required(Required const&) = delete;
		void operator=(Required const&) = delete;
	};

	/**
	 * @brief A required to initialize field specifier imitator
	 *
	 * A global variable that may be used to specify that a class field must be initialized with some value
	 * @example
	 * @code
	 * struct SomeStruct
	 * {
	 *     int field = required;
	 * };
	 *
	 * SomeStruct{};              // error: The field is uninitialized
	 * SomeStruct{.field = 0};    // ok: The field is initialized with 0
	 * @endcode
	 */
	constexpr inline Required required;

	template <class T>
	struct Box : std::unique_ptr<T>
	{
		using std::unique_ptr<T>::unique_ptr;

		template<class... argsT>
		static Box from(argsT&&... args)
		{
			return static_cast<Box&&>(std::make_unique<T>(std::forward<argsT>(args)...));
		}
	};

	template <class T>
	struct Shared : llvm::IntrusiveRefCntPtr<T>
	{
		using llvm::IntrusiveRefCntPtr<T>::IntrusiveRefCntPtr;

		template<class... argsT>
		static Shared from(argsT&&... args)
		{
			return static_cast<Shared&&>(llvm::makeIntrusiveRefCnt<T>(std::forward<argsT>(args)...));
		}
	};

	template<class... Ts>
	struct overloads : Ts... { using Ts::operator()...; };

	template<class To>
	struct static_ref_cast_t
	{
		template<class From>
		static decltype(auto) operator()(From&& from)
		{
			return static_cast<decltype(std::forward_like<From>(std::declval<To&>()))>(from);
		}
	};

	template<class To>
	static_ref_cast_t<To> static_ref_cast;

	template<class To>
	struct dynamic_ref_cast_t
	{
		template<class From>
		static decltype(auto) operator()(From&& from)
		{
			return dynamic_cast<decltype(std::forward_like<From>(std::declval<To&>()))>(from);
		}
	};

	template<class To>
	static_ref_cast_t<To> dynamic_ref_cast;

	template<class To>
	struct reinterpret_ref_cast_t
	{
		template<class From>
		static decltype(auto) operator()(From&& from)
		{
			return reinterpret_cast<decltype(std::forward_like<From>(std::declval<To&>()))>(from);
		}
	};

	template<class To>
	static_ref_cast_t<To> reinterpret_ref_cast;

	template<class To>
	struct safe_ref_cast_t
	{
		template<class From>
		static auto operator()(From&& from) -> decltype(std::forward_like<From>(std::declval<To&>()))
		{
			return from;
		}
	};

	template<class To>
	static_ref_cast_t<To> safe_ref_cast;


	template <class>
	struct owning_type_t{ static_assert(false);};

	template <class Owner, class Owning>
	struct owning_type_t<Owner Owning::*>
	{
		using owning_type = Owning;
		using owner_type = Owner;
	};

	template <class T>
	using owner_type = owning_type_t<T>::owner_type;
	template <class T>
	using owning_type = owning_type_t<T>::owning_type;


	template<auto Ptr, auto Me>
	struct RefProperty
	{
		static_assert(false);
	};

	template<class Owner, class PtrT, class MeT, PtrT Owner::* Ptr, MeT Owner::*Me>
	struct RefProperty<Ptr, Me>
	{
		Owner const* owner() const noexcept
		{
			return reinterpret_cast<Owner const*>(-reinterpret_cast<intptr_t>(reinterpret_cast<std::byte const Owner::*>(Me)) + reinterpret_cast<std::byte const*>(this));
		}

		decltype(auto) result() const noexcept
		{
			return owner()->*Ptr;
		}

		operator decltype(auto)() const noexcept
		{
			return owner()->*Ptr;
		}
	};
	struct Empty{};
}