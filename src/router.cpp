#include "router.hpp"
#include "log.hpp"

namespace schwifty::krabby {

using namespace schwifty::logger;

void router::get(std::string regex, std::set<std::string> mandatory_fields, route_t handler) {
	get(regex, handler, mandatory_fields);
}

void router::get(std::string regex, route_t handler, std::set<std::string> mandatory_fields) {
	log::info("add GET route '{}'", regex);
	add_route("GET", regex, handler, mandatory_fields);
}

void router::post(std::string regex, std::set<std::string> mandatory_fields, route_t handler) {
	post(regex, handler, mandatory_fields);
}

void router::post(std::string regex, route_t handler, std::set<std::string> mandatory_fields) {
	log::info("add POST route '{}'", regex);
	add_route("POST", regex, handler, mandatory_fields);
}

void router::delet(std::string regex, std::set<std::string> mandatory_fields, route_t handler) {
	delet(regex, handler, mandatory_fields);
}

void router::delet(std::string regex, route_t handler, std::set<std::string> mandatory_fields) {
	log::info("add DELETE route '{}'", regex);
	add_route("DELETE", regex, handler, mandatory_fields);
}

void router::put(std::string regex, std::set<std::string> mandatory_fields, route_t handler) {
	put(regex, handler, mandatory_fields);
}

void router::put(std::string regex, route_t handler, std::set<std::string> mandatory_fields) {
	log::info("add PUT route '{}'", regex);
	add_route("PUT", regex, handler, mandatory_fields);
}

void router::patch(std::string regex, std::set<std::string> mandatory_fields, route_t handler) {
	patch(regex, handler, mandatory_fields);
}

void router::patch(std::string regex, route_t handler, std::set<std::string> mandatory_fields) {
	log::info("add PATCH route '{}'", regex);
	add_route("PATCH", regex, handler, mandatory_fields);
}

void router::add_route(
    std::string method, std::string regex, route_t handler, std::set<std::string> mandatory_fields) {
	route r{handler, mandatory_fields};
	std::regex rx{regex};

	if (routes_.count(method)) {
		routes_.at(method).emplace_back(std::move(rx), std::move(r));
	} else {
		// start a fresh table
		routes_[method] = route_table_t{{std::move(rx), std::move(r)}};
	}
}

bool router::handle(http::Client *who, http::Request &request) {
	log::debug("check routes for '{}' with method {}", request.header.path, request.header.method);

	if (routes_.count(request.header.method)) {
		for (auto [rx, routing] : routes_.at(request.header.method)) {
			std::cmatch cm{};
			if (std::regex_match(request.header.path.c_str(), cm, rx)) {
				auto params = request.parse_query_params();
				for (auto &field : routing.mandatory_fields) {
					if (params.find(field) == params.end()) {
						log::warn("mandatory field '{}' was not passed in request", field);
						throw std::runtime_error(fmt::format("field '{}' is mandatory", field));
					}
				}

				routing.handler(who, request, cm, params);

				return true;
			}
		}
	}

	return false;
}

void router::clear() { routes_.clear(); }

}  // namespace schwifty::krabby