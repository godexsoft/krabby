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

server::server(uint16_t port, std::string path)
    : server_{port}, ctx_{singleton<database>::instance()["krabby"]}, script_{path} {
	// ----------------------------------------------------------------------
	server_.r_handler = [&](auto *who, http::Request &&request) {
		log::trace("request to '{}'", request.header.path);

		if (script_.handle_mountpoint(who, request))
			return;  // handled by some mountpoint

		if (script_.handle_route(who, request))
			return;  // handled by some route

		who->write(http::Response::simple_html(404, "Krabby is angry"));
	};

	server_.w_handler = [&](http::Client *who, http::WebMessage &&message) {
		if (script_.handle_websocket(who, std::move(message)))
			return;  // handled by some user scripts

		// close the connection if krabby isn't configured to support it
		return who->write(http::WebMessage(http::WebMessage::OPCODE_CLOSE, "Krabby hates you"));
	};

	server_.d_handler = [&](http::Client *who) { script_.handle_disconnect(who); };
}  // namespace schwifty::krabby

void server::websocket_response(http::Client *who, std::string msg) {
	who->write(http::WebMessage(http::WebMessage::OPCODE_TEXT, msg));
}

void server::html_response(http::Client *who, int code, std::string msg) {
	who->write(http::Response::simple_html(code, std::move(msg)));
}

void server::text_response(http::Client *who, int code, std::string msg) {
	who->write(http::Response::simple_text(code, std::move(msg)));
}

}  // namespace schwifty::krabby
