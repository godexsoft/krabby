#include "script.hpp"
#include "log.hpp"
#include "server.hpp"

namespace schwifty::krabby {
using namespace schwifty::logger;
using json = nlohmann::json;

script_engine::script_engine(std::filesystem::path path) : path_{path}, router_{}, mountpoints_{} { reload(); }

void script_engine::reload() {
	router_.clear();
	lua_ = std::make_shared<sol::state>();

	log::info("loading up Lua extension engine with root path '{}'", path_.string());
	lua_->open_libraries(sol::lib::base, sol::lib::package);

	register_types();
	setup_router_api();
	setup_mountpoint_api();

	// export global objects to lua
	lua_->set("template", sol::var(std::ref(singleton<inja::Environment>::instance())));

	load_extensions(path_);
}

void script_engine::register_types() {
	sol::usertype<crab::Timer> timer_type =
	    lua_->new_usertype<crab::Timer>("timer", sol::constructors<crab::Timer(crab::Handler &&)>());
	timer_type["once"]   = static_cast<void (crab::Timer::*)(double)>(&crab::Timer::once);
	timer_type["cancel"] = &crab::Timer::cancel;

	sol::usertype<http::Client> client_type = lua_->new_usertype<http::Client>("client", sol::no_constructor);

	sol::usertype<http::Request> request_type = lua_->new_usertype<http::Request>("request", sol::no_constructor);
	request_type["header"]                    = sol::readonly_property(&http::Request::header);
	request_type["body"]                      = sol::readonly_property(&http::Request::body);

	sol::usertype<http::RequestHeader> request_header_type = lua_->new_usertype<http::RequestHeader>(
	    "request_header", sol::no_constructor, sol::base_classes, sol::bases<http::RequestResponseHeader>());
	request_header_type["method"]              = sol::readonly_property(&http::RequestHeader::method);
	request_header_type["path"]                = sol::readonly_property(&http::RequestHeader::path);
	request_header_type["query_string"]        = sol::readonly_property(&http::RequestHeader::query_string);
	request_header_type["basic_authorization"] = sol::readonly_property(&http::RequestHeader::basic_authorization);
	request_header_type["host"]                = sol::readonly_property(&http::RequestHeader::host);
	request_header_type["origin"]              = sol::readonly_property(&http::RequestHeader::origin);
	request_header_type["sec_websocket_key"]   = sol::readonly_property(&http::RequestHeader::sec_websocket_key);
	request_header_type["sec_websocket_version"] =
	    sol::readonly_property(&http::RequestHeader::sec_websocket_version);
	request_header_type["is_websocket_upgrade"] = &http::RequestHeader::is_websocket_upgrade;
	request_header_type["uri"] = sol::property(&http::RequestHeader::get_uri, &http::RequestHeader::set_uri);

	sol::usertype<http::RequestResponseHeader> request_response_header_type =
	    lua_->new_usertype<http::RequestResponseHeader>("request_response_header_type", sol::no_constructor);
	request_response_header_type["headers"] = sol::readonly_property(&http::RequestResponseHeader::headers);
	request_response_header_type["content_type_mime"] =
	    sol::readonly_property(&http::RequestResponseHeader::content_type_mime);
	request_response_header_type["content_type_suffix"] =
	    sol::readonly_property(&http::RequestResponseHeader::content_type_suffix);

	sol::usertype<http::Header> header_type = lua_->new_usertype<http::Header>("header", sol::no_constructor);
	header_type["name"]                     = &http::Header::name;
	header_type["value"]                    = &http::Header::value;

	sol::usertype<inja::Environment> inja_env_type =
	    lua_->new_usertype<inja::Environment>("inja_environment", sol::no_constructor);
	inja_env_type["render_file"] = &inja::Environment::render_file;

	// clang-format off
    sol::usertype<json> json_type = lua_->new_usertype<json>(
        "json", "new", sol::constructors<json()>(), 
        "parse", [](const std::string &value) { return json::parse(value); }); 

	json_type["str"] = sol::overload(
        [](json &j, const std::string &key) { return j[key].get<std::string>(); },
        [](json &j) { return j.get<std::string>(); },
		[](json &j, const std::string &key, const std::string &value) { j[key] = value; } );

    json_type["int"] = sol::overload(
        [](json &j, const std::string &key) { return j[key].get<int>(); },
        [](json &j) { return j.get<int>(); },
		[](json &j, const std::string &key, int value) { j[key] = value; } );

    json_type["dbl"] = sol::overload(
        [](json &j, const std::string &key) { return j[key].get<double>(); },
        [](json &j) { return j.get<double>(); },
		[](json &j, const std::string &key, double value) { j[key] = value; } );
    
    json_type["obj"] = [](json &j, const std::string &key) { return j[key]; };
	// clang-format on

	// global static functions
	lua_->set_function("html_response", &server::html_response);
	lua_->set_function("text_response", &server::text_response);

	lua_->set_function("hash_password", &hash_password);
	lua_->set_function("hash_sha1", &hash_sha1);
	lua_->set_function("hmac_sha1", &hmac_sha1);
}

void script_engine::setup_router_api() {
	using lua_route_t = sol::function;
	lua_->set_function("Get", [&](std::string path, router::fields_t required_fields, lua_route_t func) {
		router_.get(path, required_fields, [func](auto *who, auto &req, auto &matches, auto &params) {
			// forward it to lua
			std::vector<std::string> m{matches.begin(), matches.end()};
			func(who, req, m, params);
		});
		log::info("LUA: added Get route '{}'", path);
	});

	lua_->set_function("Post", [&](std::string path, router::fields_t required_fields, lua_route_t func) {
		router_.post(path, required_fields, [func](auto *who, auto &req, auto &matches, auto &params) {
			// forward it to lua
			std::vector<std::string> m{matches.begin(), matches.end()};
			func(who, req, m, params);
		});
		log::info("LUA: added Post route '{}'", path);
	});
}

void script_engine::setup_mountpoint_api() {
	lua_->set_function("Mount", [&](std::string path, std::string fs_path) {
		mountpoints_.emplace_back(path, path_ / fs_path);
		log::info("LUA: added mountpoint '{}' -> '{}'", path, fs_path);
	});
}

void script_engine::load_extensions(std::filesystem::path path) {
	log::debug("loading extensions from '{}'", path.string());
	for (auto &p : std::filesystem::directory_iterator(path)) {
		if (p.is_regular_file()) {
			if (p.path().extension().string() == crab::Literal{".lua"}) {
				sol::load_result lr = lua_->load_file(p.path());
				if (!lr.valid()) {
					sol::error err = lr;
					throw std::runtime_error(
					    fmt::format("compilation of '{}' failed: {}", p.path().string(), err.what()));
				} else {
					sol::protected_function code       = lr;
					sol::protected_function_result res = code();

					if (!res.valid()) {
						sol::error err = res;
						throw std::runtime_error(
						    fmt::format("execution of '{}' failed: {}", p.path().string(), err.what()));
					}
				}
			}
		} else if (p.is_directory()) {
			load_extensions(p.path());
		}
	}
}

bool script_engine::handle_mountpoint(http::Client *who, http::Request &request) {
	for (auto &mnt : mountpoints_) {
		log::trace("checking mountpoint...");
		if (mnt.handle(who, request))
			return true;
	}
	return false;
}

bool script_engine::handle_route(http::Client *who, http::Request &request) { return router_.handle(who, request); }

}  // namespace schwifty::krabby