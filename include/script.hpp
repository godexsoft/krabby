#pragma once

#include "router.hpp"
#include "singleton.hpp"
#include <crab/crab.hpp>
#include <filesystem>
#include <memory>
#include <sol/sol.hpp>
#include <unordered_map>

namespace schwifty::krabby
{

  namespace http = crab::http;

  class server;

  class script_engine
  {
  public:
    script_engine(std::filesystem::path path, server &server);
    void reload();

    bool handle_route(http::Client *who, http::Request &request);

  private:
    void register_types();
    void setup_router_api();

    void load_extensions(std::filesystem::path path);

    std::filesystem::path path_;
    server &server_; // expected to stay the same throughout server lifetime

    router router_;
    std::shared_ptr<sol::state> lua_;
  };

} // namespace schwifty::krabby