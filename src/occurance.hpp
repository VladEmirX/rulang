#pragma once
#include <stdexcept>
#include <string>
#include <format>
#include <boost/describe.hpp>
#include <boost/describe/enum_to_string.hpp>

namespace Ru::occurance
{
	struct occurance_position
	{
		virtual std::string to_string() const = 0;
		virtual ~occurance_position() = default;
	};

	struct position_in_text final: occurance_position
	{
		size_t line = 0, line_end = line, pos = 0, pos_end = pos;
		std::string to_string() const override
		{
			return 
				  line != line_end
				? std::format("at {}:{} .. {}:{}", line, pos, line_end, pos_end)
				: pos != pos_end
				? std::format("at {} : {}..{}", line, pos, pos_end)
				: std::format("at {}:{}", line, pos);
		}
	};

	struct position_in_file_at final: occurance_position
	{
		std::string file_name;
		occurance_position const* at;

		std::string to_string() const override
		{
			return "in " + file_name + at->to_string();
		}
	};

	enum class occurance_type : uint8_t
	{
		Message,
		Warning,
		Error,
	};
	BOOST_DESCRIBE_ENUM
	(
		occurance_type,

		Message,
		Warning,
		Error,
	)
	using enum occurance_type;

	struct occurance : std::runtime_error
	{
		using at_type = occurance_position;
		using type_type = occurance_type;
		//using enum type_type;
		virtual std::string text() const noexcept = 0;
		virtual at_type const& at() const noexcept = 0;		
		virtual type_type type() const noexcept = 0;
	protected:
		occurance() 
			: runtime_error(std::format
				( "{} occured {}: {}"
				, boost::describe::enum_to_string(type(), "Something")
				, at().to_string()
				, text()
				)){};
	};

	struct nested_occurance : occurance
	{
		using nested_type = occurance;
		virtual nested_type const& nested() const noexcept = 0;
	};
}

