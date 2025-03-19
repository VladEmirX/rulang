#include <boost/nowide/args.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/nowide/iostream.hpp>
#include <direct.h>
#include "rulang.hpp"

using namespace std::string_literals;

void rulang_main(int argc, char *argv[]) try
{
	boost::nowide::args _args(argc, argv);

}
catch (std::exception const& x)
{
	boost::nowide::cerr << "Internal error: " << x.what();
	throw;
}

namespace
{

	class global_presets
	{
		static struct global_presets _;
		global_presets()
		{
			boost::locale::localization_backend_manager locale_manager{};
			locale_manager.select("icu"s);
			auto locale_generator = boost::locale::generator(locale_manager);
			std::locale::global(locale_generator("EN_us.UTF-8"));
			boost::nowide::nowide_filesystem();
		}
	};
	global_presets global_presets::_ = {};
}

extern const boost::filesystem::path cwd = []
{
	char cwd[_MAX_PATH]{};
	(void)_getcwd(cwd, _MAX_PATH);

	return boost::filesystem::path(cwd);
}();
