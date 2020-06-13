#include <csignal>
#include <cxxopts.hpp>

#include "script.hpp"
#include "server.hpp"
#include "singleton.hpp"

using namespace schwifty::logger;
using namespace schwifty::krabby;

void signal_handler(int signal) { std::exit(0); }

int main(int argc, char *argv[]) {
	uint16_t port{8080};
	std::string data_path{"./"};
	bool logging{false};

	try {
		cxxopts::Options options("krabby", "Scriptable http/ws api server");

		// clang-format off
        options.add_options()
            ("p,port", "TCP port", cxxopts::value<uint16_t>(port))
            ("path", "Data path (can also be specified as first argument)", cxxopts::value<std::string>(), "path")
            ("h,help", "Help message")
        ;
		// clang-format on

		if constexpr (schwifty::krabby::EnableLog) {
			options.add_options()("l,logging", "Log to stderr", cxxopts::value<bool>(logging));
		}

		options.positional_help("[directory]").show_positional_help();
		options.parse_positional({"path"});
		auto result = options.parse(argc, argv);

		if (result.count("help")) {
			fmt::print(options.help({""}));
			return 0;
		}

		if (result.count("path")) {
			data_path = append_trailing_slash(result["path"].as<std::string>());
		}
	} catch (const cxxopts::OptionException &e) {
		log::fatal("Error parsing options: {}", e.what());
		return 1;
	}

	log::enable(logging);
	log::level(loglevel::trace);

	log::info("data path: {}", data_path);
	log::info("service port: {}", port);
	std::cout << "Running on port {" << port << "} data path {" << data_path << "}" << std::endl;

	std::signal(SIGINT, signal_handler);
	crab::RunLoop runloop;

	singleton<database> db{data_path};
	singleton<inja::Environment> env{data_path};
	env.set_lstrip_blocks(true);
	env.set_trim_blocks(true);

	server app{port, data_path};

	runloop.run();
	return 0;
}