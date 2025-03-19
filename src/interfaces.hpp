#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <ostream>
#include <memory>
#pragma once

#ifdef _MSVC_LANG
#define INTERFACE __interface
#else
#define INTERFACE struct
#endif

namespace Ru
{
	class Object : public llvm::RefCountedBase<Object>
	{
		Object(Object const&) noexcept = default;
	public:
		virtual ~Object() = default;
	protected:
		Object() noexcept = default;
	};

	INTERFACE ICloneable
	{
		[[nodiscard, gnu::returns_nonnull]]
		virtual ICloneable * rawClone() const& = 0;

		template <class Self>
		[[nodiscard]] auto clone(this Self const& self)
		{
			return std::unique_ptr<Self>(self.rawClone());
		}
	};

	INTERFACE IStringConvertible
	{
	public:
		[[nodiscard]] virtual std::string toString() const = 0;
		friend std::ostream& operator<<(std::ostream& out, IStringConvertible const& self)
		{
			return out << self.toString();
		}
	};
}