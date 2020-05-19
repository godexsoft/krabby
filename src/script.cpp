#include "script.hpp"
#include "log.hpp"
#include "server.hpp"

namespace schwifty::krabby {
using namespace schwifty::logger;
using json = nlohmann::json;

script_engine::script_engine(std::filesystem::path path) : path_{path}, swap_timer_{[this]() { swap_context(); }} {
	try {
		reload();
	} catch (std::exception &e) {
		log::warn("STARTUP FAILED:\n---\n{}\n---", e.what());
		std::exit(-1);
	}
}

void script_engine::reload() {
	staging_ctx_ = std::make_shared<scripting_context>();

	log::debug("loading up Lua scripts engine with root path '{}'", path_.string());
	staging_ctx_->lua_.open_libraries(
	    sol::lib::base, sol::lib::os, sol::lib::table, sol::lib::package, sol::lib::string);

	register_types();
	setup_generic_api();
	setup_router_api();
	setup_mountpoint_api();
	setup_websocket_api();
	setup_client_api();

	// export global objects to lua
	staging_ctx_->lua_.set("template", sol::var(std::ref(singleton<inja::Environment>::instance())));
	staging_ctx_->lua_.set("storage", sol::var(std::ref(singleton<database>::instance().storage())));

	load_extensions(path_);  // if this will throw, swap will not be scheduled
	swap_timer_.once(0);     // execute ASAP
}

void script_engine::swap_context() {
	log::debug("Swapping scripting context");
	master_ctx_  = staging_ctx_;
	staging_ctx_ = nullptr;
	log::info("Krabby scripting engine is operational now");
}

void script_engine::register_types() {
	sol::usertype<crab::Timer> timer_type =
	    staging_ctx_->lua_.new_usertype<crab::Timer>("timer", sol::constructors<crab::Timer(crab::Handler &&)>());
	timer_type["once"]   = static_cast<void (crab::Timer::*)(double)>(&crab::Timer::once);
	timer_type["cancel"] = &crab::Timer::cancel;

	sol::usertype<http::Client> client_type =
	    staging_ctx_->lua_.new_usertype<http::Client>("client", sol::no_constructor);
	client_type["upgrade"] = [](http::Client &self) { self.web_socket_upgrade(); };
	client_type["id"] =
	    sol::readonly_property([](http::Client &self) { return fmt::format("{}", static_cast<void *>(&self)); });

	sol::usertype<http::Request> request_type =
	    staging_ctx_->lua_.new_usertype<http::Request>("request", sol::no_constructor);
	request_type["header"] = sol::readonly_property(&http::Request::header);
	request_type["body"]   = sol::readonly_property(&http::Request::body);

	sol::usertype<http::Response> response_type =
	    staging_ctx_->lua_.new_usertype<http::Response>("response", sol::no_constructor);
	response_type["header"] = sol::readonly_property(&http::Response::header);
	response_type["body"]   = sol::readonly_property(&http::Response::body);

	sol::usertype<http::ClientRequestSimple> crequest_type =
	    staging_ctx_->lua_.new_usertype<http::ClientRequestSimple>("client_request", sol::no_constructor);
	crequest_type["cancel"] = &http::ClientRequestSimple::cancel;
	crequest_type["isOpen"] = sol::readonly_property(&http::ClientRequestSimple::is_open);

	sol::usertype<http::WebMessage> wm_type =
	    staging_ctx_->lua_.new_usertype<http::WebMessage>("webmessage", sol::no_constructor);
	wm_type["body"] = sol::readonly_property(&http::WebMessage::body);

	sol::usertype<http::RequestHeader> request_header_type = staging_ctx_->lua_.new_usertype<http::RequestHeader>(
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
	    staging_ctx_->lua_.new_usertype<http::RequestResponseHeader>(
	        "request_response_header_type", sol::no_constructor);
	request_response_header_type["headers"] = sol::readonly_property(&http::RequestResponseHeader::headers);
	request_response_header_type["content_type_mime"] =
	    sol::readonly_property(&http::RequestResponseHeader::content_type_mime);
	request_response_header_type["content_type_suffix"] =
	    sol::readonly_property(&http::RequestResponseHeader::content_type_suffix);

	sol::usertype<http::Header> header_type =
	    staging_ctx_->lua_.new_usertype<http::Header>("header", sol::no_constructor);
	header_type["name"]  = &http::Header::name;
	header_type["value"] = &http::Header::value;

	sol::usertype<inja::Environment> inja_env_type =
	    staging_ctx_->lua_.new_usertype<inja::Environment>("inja_environment", sol::no_constructor);
	inja_env_type["render_file"] = &inja::Environment::render_file;

	using strvec_t = std::vector<std::string>;
	sol::usertype<strvec_t> stringvec_type =
	    staging_ctx_->lua_.new_usertype<strvec_t>("string_vector", "new", sol::constructors<strvec_t()>());
	stringvec_type["push_back"] = [](strvec_t &v, const std::string &value) { v.push_back(value); };
	stringvec_type[sol::meta_function::static_index] = [](strvec_t &v, const int &idx) { return v.at(idx - 1); };
	stringvec_type[sol::meta_function::index]        = [](strvec_t &v, const int &idx) { return v.at(idx - 1); };
	stringvec_type["erase"]                          = [](strvec_t &v, const std::string &value) {
        v.erase(std::remove(std::begin(v), std::end(v), value), std::end(v));
	};

	// clang-format off
    sol::usertype<json> json_type = staging_ctx_->lua_.new_usertype<json>(
        "json", "new", sol::constructors<json()>(), 
				"array", []() { return json::array(); },
        		"parse", [](const std::string &value) { return json::parse(value); }); 

	json_type["push_back"] = sol::overload(
		[](json &j, const json& value) { j.push_back(value); },
		[](json &j, const std::string& value) { j.push_back(value); },
		[](json &j, const double& value) { j.push_back(value); },
		[](json &j, const bool& value) { j.push_back(value); },
		[](json &j, const int& value) { j.push_back(value); } );

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
    
	json_type["bool"] = sol::overload(
        [](json &j, const std::string &key) { return j[key].get<bool>(); },
        [](json &j) { return j.get<bool>(); },
		[](json &j, const std::string &key, bool value) { j[key] = value; } );
    
    json_type["obj"] = sol::overload(
        [](json &j, const std::string &key) { return j[key]; },        
		[](json &j, const std::string &key, const json& value) { j[key] = value; } );

    json_type["vec"] = sol::overload(        
		[](json &j, const std::string &key, const strvec_t& value) { j[key] = value; },
		[](json &j, const std::string &key, const std::vector<int>& value) { j[key] = value; },
		[](json &j, const std::string &key, const std::vector<double>& value) { j[key] = value; },
		[](json &j, const std::string &key, const std::vector<bool>& value) { j[key] = value; } );

	json_type["empty"] = sol::readonly_property(&json::empty);
	json_type["dump"] = [](json &j) { return j.dump(); };

	sol::usertype<sql_bridge::context> sql_ctx_type =
	    staging_ctx_->lua_.new_usertype<sql_bridge::context>("sqlcontext", sol::no_constructor);

	using kvstore = sql_bridge::local_storage<sql_bridge::sqlite_adapter>;
	sol::usertype<kvstore> sql_local_storage_type =
	    staging_ctx_->lua_.new_usertype<kvstore>("sqllocalstore", sol::no_constructor);

	sql_local_storage_type["save"] = sol::overload(
		static_cast<void(kvstore::*)(const std::string&, const std::string&)const>(&kvstore::save),
		static_cast<void(kvstore::*)(const std::string&, const strvec_t&)const>(&kvstore::save),
		[](kvstore& store, const std::string& key, const json& data) {  
			store.save(key, data.dump()); 
		}
	);

	sql_local_storage_type["remove"] = sol::overload(
		[](kvstore& store, const std::string& key) {  
			store.remove<std::string>(key);
		},
		[](kvstore& store, const std::string& lst, const std::string& itm) {			
        	auto data = store.load(lst, strvec_t{});
			if (!data.empty()) {
				data.erase(std::remove(std::begin(data), std::end(data), itm), std::end(data));
				store.save(lst, data);
			}
		},
		[](kvstore& store, const std::string& lst, const json& itm) {			
        	auto data = store.load(lst, strvec_t{});
			if (!data.empty()) {
				data.erase(std::remove(std::begin(data), std::end(data), itm.dump()), std::end(data));
				store.save(lst, data);
			}
		}
	);

	sql_local_storage_type["load"] = sol::overload(
		static_cast<std::string(kvstore::*)(const std::string&, const std::string&)const>(&kvstore::load),
		static_cast<strvec_t(kvstore::*)(const std::string&, const strvec_t&)const>(&kvstore::load),
		[](kvstore& store, const std::string& key, const json& def) {  
			return json::parse(store.load(key, def.dump()));
		}
	);
	// clang-format on	
}

void script_engine::setup_generic_api() {
	// global static functions
	staging_ctx_->lua_.set_function("respond", &server::response);
	staging_ctx_->lua_.set_function("respond_html", &server::html_response);
	staging_ctx_->lua_.set_function("respond_text", &server::text_response);
	staging_ctx_->lua_.set_function("respond_msg", &server::websocket_response);

	staging_ctx_->lua_.set_function("generate_key", &generate_key);
	staging_ctx_->lua_.set_function("hash_sha1", &hash_sha1);
	staging_ctx_->lua_.set_function("hmac_sha1", &hmac_sha1);
	staging_ctx_->lua_.set_function("escape_html", &escape_html);
	staging_ctx_->lua_.set_function("string_to_hex", &string_to_hex);

	using lua_disconnect_handler_t = sol::function;
	staging_ctx_->lua_.set_function("Disconnect", [&](lua_disconnect_handler_t func) {
		staging_ctx_->disconnect_handlers_.emplace_back(func);
		log::info("LUA: added disconnect handler");
	});

	staging_ctx_->lua_.set_function("Reload", [&]() -> std::string {		
		try {
			reload();
			return std::string{};
		} catch(std::exception& e) {
			log::warn("RELOAD FAILED:\n---\n{}\n---\n", e.what());			
			return e.what();
		}
	});
}

void script_engine::setup_router_api() {
	using lua_route_t = sol::function;
	staging_ctx_->lua_.set_function("Get", [&](std::string path, router::fields_t required_fields, lua_route_t func) {
		staging_ctx_->router_.get(path, required_fields, [func](auto *who, auto &req, auto &matches, auto &params) {
			// forward it to lua
			std::vector<std::string> m{matches.begin(), matches.end()};
			func(who, req, m, params);
		});
		log::info("LUA: added Get route '{}'", path);
	});

	staging_ctx_->lua_.set_function("Post", [&](std::string path, router::fields_t required_fields, lua_route_t func) {
		staging_ctx_->router_.post(path, required_fields, [func](auto *who, auto &req, auto &matches, auto &params) {
			// forward it to lua
			std::vector<std::string> m{matches.begin(), matches.end()};
			func(who, req, m, params);
		});
		log::info("LUA: added Post route '{}'", path);
	});

	staging_ctx_->lua_.set_function("Delete", [&](std::string path, router::fields_t required_fields, lua_route_t func) {
		staging_ctx_->router_.delet(path, required_fields, [func](auto *who, auto &req, auto &matches, auto &params) {
			// forward it to lua
			std::vector<std::string> m{matches.begin(), matches.end()};
			func(who, req, m, params);
		});
		log::info("LUA: added Delete route '{}'", path);
	});

	staging_ctx_->lua_.set_function("Put", [&](std::string path, router::fields_t required_fields, lua_route_t func) {
		staging_ctx_->router_.put(path, required_fields, [func](auto *who, auto &req, auto &matches, auto &params) {
			// forward it to lua
			std::vector<std::string> m{matches.begin(), matches.end()};
			func(who, req, m, params);
		});
		log::info("LUA: added Put route '{}'", path);
	});

	staging_ctx_->lua_.set_function("Patch", [&](std::string path, router::fields_t required_fields, lua_route_t func) {
		staging_ctx_->router_.patch(path, required_fields, [func](auto *who, auto &req, auto &matches, auto &params) {
			// forward it to lua
			std::vector<std::string> m{matches.begin(), matches.end()};
			func(who, req, m, params);
		});
		log::info("LUA: added Patch route '{}'", path);
	});
}

void script_engine::setup_mountpoint_api() {
	staging_ctx_->lua_.set_function("Mount", [&](std::string path, std::string fs_path) {
		staging_ctx_->mountpoints_.emplace_back(path, path_ / fs_path);
		log::info("LUA: added mountpoint '{}' -> '{}'", path, fs_path);
	});
}

void script_engine::setup_websocket_api() {
	using lua_ws_handler_t = sol::function;
	staging_ctx_->lua_.set_function("Msg", [&](lua_ws_handler_t func) {
		staging_ctx_->ws_handlers_.emplace_back(func);
		log::info("LUA: added ws handler");
	});
}

void script_engine::setup_client_api() {
	using lua_creq_handler_t = sol::function;
	staging_ctx_->lua_.set_function("ClientGet", [&](const std::string& url, lua_creq_handler_t on_res, lua_creq_handler_t on_err) {
		auto simple = std::make_shared<http::ClientRequestSimple>([on_res](http::Response &&resp) { 
				on_res(resp);
			},
            [on_err](const std::string &err) { 
				on_err(err);
			});
        
		simple->send_get(url);		
		return simple;
	});
}

void script_engine::load_extensions(std::filesystem::path path) {
	log::debug("loading scripts from '{}'", path.string());
	for (auto &p : std::filesystem::directory_iterator(path)) {
		if (p.is_regular_file()) {
			if (p.path().extension().string() == crab::Literal{".lua"}) {
				sol::load_result lr = staging_ctx_->lua_.load_file(p.path());
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
	for (auto &mnt : master_ctx_->mountpoints_) {
		if (mnt.handle(who, request))
			return true;
	}
	return false;
}

bool script_engine::handle_route(http::Client *who, http::Request &request) { return master_ctx_->router_.handle(who, request); }

bool script_engine::handle_websocket(http::Client *who, http::WebMessage &&msg) {
	for (auto &ws : master_ctx_->ws_handlers_) {
		if (ws(who, msg))
			return true;
	}
	return false;
}

void script_engine::handle_disconnect(http::Client *who) {
	for (auto &dc : master_ctx_->disconnect_handlers_) {
		dc(who);
	}
}

}  // namespace schwifty::krabby