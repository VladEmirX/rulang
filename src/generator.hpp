//created this because <boost/cobald/...> didn't want to link and <generator> is not available yet
#pragma once
#include <coroutine>
#include <expected>


namespace Ru
{
	struct returned_generator_error : std::logic_error
	{
		returned_generator_error() : logic_error("Running a returned generator") {};
	};

	template <class yield_type>
	struct generator;

	template <class yieldT>
	struct generator_promise
	{
		using yield_type = yieldT;
		using yield_ref = yieldT&&;
		using yield_ptr = std::remove_reference_t<yieldT>*;
		
		generator<yield_type> get_return_object() noexcept;
		std::suspend_never    initial_suspend() noexcept { return {}; }
		std::suspend_always   final_suspend() noexcept { return {}; }

		void return_void() noexcept { _yielded = std::unexpected(std::make_exception_ptr(returned_generator_error())); }
		std::suspend_always yield_value(yield_ref value) { _yielded = &value; return {}; }

		void unhandled_exception() noexcept { _yielded = std::unexpected(std::current_exception()); }
		yield_ptr operator->()
		{ 
			if (not _yielded) std::rethrow_exception(_yielded.error()); 
			return *_yielded;
		}
		yield_ref operator*()
		{
			if (not _yielded) std::rethrow_exception(_yielded.error());
			return std::forward<yield_type>(**_yielded);
		}
	private:
		std::expected<yield_ptr, std::exception_ptr> _yielded;
	};

	template <class yieldT>
	struct generator
	{
		using yield_type = yieldT;
		using promise_type = generator_promise<yield_type>;
		using handle_type = std::coroutine_handle<promise_type>;
		friend generator<yield_type> promise_type::get_return_object() noexcept;

		void swap(generator& other) noexcept
		{
			std::swap(this->_promise, other._promise);
		}

		~generator() { if (_promise) _promise.destroy(); }
		generator(generator const&) = delete;
		generator(generator&& other) noexcept { this->swap(other); }
		void operator=(generator const&) = delete;
		generator& operator=(generator&& other)& noexcept { this->swap(other); return *this; }

		struct iterator
		{
			decltype(auto) operator->() const { return self->_promise.promise(); }
			decltype(auto) operator*() const { return *self->_promise.promise(); }
			explicit operator bool() const { return self and not self->_promise.done(); }
			bool operator !=(iterator) const { return (bool) *this; }
			decltype(auto) operator++() noexcept { self->_promise(); return *this; }
			void operator++(int) { ++*this; }
			explicit iterator(generator& self) : self(&self) { }
			explicit iterator() : self(nullptr) {}
		private:
			generator const* self;
		};
		iterator begin() noexcept
		{
			return iterator(*this);
		}
		constexpr static iterator end() noexcept { return iterator(); };
		//constexpr iterator end() const noexcept { return {}; };

	private:
		constexpr generator() noexcept = default;
		handle_type _promise;
	};

	template <class yield_type>
	inline generator<yield_type> generator_promise<yield_type>::get_return_object() noexcept
	{
		generator<yield_type> g;
		g._promise = generator<yield_type>::handle_type::from_promise(*this);
		return g;
	}
}
