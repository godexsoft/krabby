#include "server.hpp"
#include "log.hpp"
#include "script.hpp"
#include "singleton.hpp"
#include "util.hpp"

namespace schwifty::krabby {
using namespace schwifty::logger;

using namespace std;
using namespace inja;
using json = nlohmann::json;

server::server(uint16_t port)
    : server_{port}, ctx_{singleton<database>::instance()["krabby"]}, router_{}, connections_{} {
	// ----------------------------------------------------------------------
	server_.r_handler = [&](auto *who, http::Request &&request) {
		log::trace("request to '{}'", request.header.path);

		// TODO: all mountpoints

		if (singleton<script_engine>::instance().handle_route(who, request))
			return;  // handled by some route

		who->write(http::Response::simple_html(404, "Krabby is angry"));
	};

	server_.w_handler = [&](http::Client *who, http::WebMessage &&message) {
		if (message.is_binary()) {
			// TODO
			return;
		} else {
			if (connections_.count(who)) {
				// TODO
				return server::json_response(who, 404, "Not implemented");
			}
		}
	};

	server_.d_handler = [&](auto *who) {
		if (connections_.count(who)) {
			log::info("connection {} closing", static_cast<void *>(who));
			connections_.erase(who);
		}
	};
}  // namespace schwifty::krabby

void server::json_response(http::Client *who, int code, std::string msg) {
	json j{{"e", code}};
	if (!msg.empty()) {
		j["m"] = msg;
	}
	who->write(http::WebMessage(1, j.dump()));
}

void server::html_response(http::Client *who, int code, std::string msg) {
	who->write(http::Response::simple_html(code, std::move(msg)));
}

void server::text_response(http::Client *who, int code, std::string msg) {
	who->write(http::Response::simple_text(code, std::move(msg)));
}

}  // namespace schwifty::krabby
