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

server::server(uint16_t port, std::string path) : server_{port}, script_{path} {
	// ----------------------------------------------------------------------
	server_.r_handler = [&](auto *who, http::Request &&request) {
		log::trace("request to '{}'", request.header.path);

		if (script_.handle_mountpoint(who, request))
			return;  // handled by some mountpoint

		if (script_.handle_route(who, request))
			return;  // handled by some route

		who->write(http::Response::simple_html(404, "Krabby is angry"));
	};

}  // namespace schwifty::krabby

void server::websocket_response(http::Client *who, std::string msg) {
	who->write(http::WebMessage(http::WebMessage::OPCODE_TEXT, std::move(msg)));
}

void server::response(http::Client *who, int code, std::string content_type, std::string data) {
	http::Response res;

	res.header.set_content_type(content_type);
	res.set_body(std::move(data));

	who->write(std::move(res));
}

void server::html_response(http::Client *who, int code, std::string msg) {
	who->write(http::Response::simple_html(code, std::move(msg)));
}

void server::text_response(http::Client *who, int code, std::string msg) {
	who->write(http::Response::simple_text(code, std::move(msg)));
}

}  // namespace schwifty::krabby
