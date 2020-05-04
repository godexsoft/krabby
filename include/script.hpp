#pragma once

#include <crab/crab.hpp>
#include <filesystem>
#include <sol/sol.hpp>
#include "mountpoint.hpp"
#include "router.hpp"

namespace schwifty::krabby {

namespace http = crab::http;

class script_engine {
public:
	script_engine(std::filesystem::path path);
	void reload();

	bool handle_route(http::Client *who, http::Request &request);
	bool handle_mountpoint(http::Client *who, http::Request &request);
	bool handle_websocket(http::Client *who, http::WebMessage &&msg);
	void handle_disconnect(http::Client *who);

private:
	struct scripting_context {
		scripting_context(std::filesystem::path path) {}

		router router_;
		std::vector<mountpoint> mountpoints_;
		std::vector<ws_handler_t> ws_handlers_;
		std::vector<dc_handler_t> disconnect_handlers_;
		sol::state lua_;
	};

	void swap_context();

	void register_types();
	void setup_generic_api();
	void setup_router_api();
	void setup_mountpoint_api();
	void setup_websocket_api();

	void load_extensions(std::filesystem::path path);

	std::filesystem::path path_;
	crab::Timer swap_timer_;
	std::shared_ptr<scripting_context> staging_ctx_;
	std::shared_ptr<scripting_context> main_ctx_;
};

}  // namespace schwifty::krabby