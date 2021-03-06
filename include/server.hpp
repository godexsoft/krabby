#pragma once

#include <fstream>
#include <iostream>
#include <set>
#include <unordered_map>

#include <crab/crab.hpp>
#include <inja/inja.hpp>

#include "database.hpp"
#include "log.hpp"
#include "mountpoint.hpp"
#include "router.hpp"
#include "script.hpp"

namespace schwifty::krabby {

namespace http = crab::http;
namespace sql  = sql_bridge;

class server {
public:
	explicit server(uint16_t port, std::string path);

	static void response(http::Client *who, int code, std::string content_type, std::string data);
	static void html_response(http::Client *who, int code, std::string msg = std::string{});
	static void text_response(http::Client *who, int code, std::string msg = std::string{});
	static void websocket_response(http::Client *who, std::string msg = std::string{});

private:
	http::Server server_;   // http service provider
	script_engine script_;  // main scripting interface
};

}  // namespace schwifty::krabby