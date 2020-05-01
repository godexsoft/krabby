#include <cxxopts.hpp>

#include "script.hpp"
#include "server.hpp"
#include "singleton.hpp"

using namespace schwifty::logger;
using namespace schwifty::krabby;

int main(int argc, char *argv[]) {
	uint16_t port{8888};
	std::string storage_path{"./"};
	bool logging{false};

	try {
		cxxopts::Options options("krabby", "Scriptable http/ws api server");

		// clang-format off
        options.add_options()
            ("p,port", "TCP port", cxxopts::value<uint16_t>(port))        
            ("s,storage", "Storage root path", cxxopts::value<std::string>(storage_path))
            ("h,help", "Help message")
        ;
		// clang-format on

		if constexpr (schwifty::krabby::EnableLog) {
			options.add_options()("l,logging", "Log to stderr", cxxopts::value<bool>(logging));
		}

		auto result = options.parse(argc, argv);

		if (result.count("help")) {
			fmt::print(options.help({""}));
			return 0;
		}
	} catch (const cxxopts::OptionException &e) {
		log::fatal("Error parsing options: {}", e.what());
		return 1;
	}

	log::enable(logging);
	log::level(loglevel::trace);

	log::info("storage path: {}", storage_path);
	log::info("service port: {}", port);

	crab::RunLoop runloop;

	singleton<database> db{storage_path};
	singleton<inja::Environment> env{"./"};
	env.set_lstrip_blocks(true);
	env.set_trim_blocks(true);

	server app{port};
	singleton<script_engine> ext{"./", app};

	runloop.run();
	return 0;
}