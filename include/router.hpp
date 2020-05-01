#pragma once

#include <crab/crab.hpp>

#include <functional>
#include <map>
#include <regex>
#include <set>
#include <unordered_map>

#include "types.hpp"
#include "util.hpp"

namespace schwifty::krabby
{

  namespace http = crab::http;

  class router
  {
  public:
    using route_t = std::function<void(http::Client *, http::Request &, std::cmatch &, kv_map_t &)>;
    using fields_t = std::set<std::string>;

    struct route
    {
      route_t handler;
      fields_t mandatory_fields;
    };

    using route_table_t = std::vector<std::tuple<std::regex, route>>;

    void get(std::string regex, route_t handler, fields_t mandatory_fields = {});
    void get(std::string regex, fields_t mandatory_fields, route_t handler);

    void post(std::string regex, std::set<std::string> mandatory_fields, route_t handler);
    void post(std::string regex, route_t handler, fields_t mandatory_fields = {});

    bool handle(http::Client *who, http::Request &request);
    void clear();

  private:
    void add_route(std::string method, std::string regex, route_t handler, std::set<std::string> mandatory_fields);

    // routes are grouped per Method (POST, GET, etc.)
    std::map<std::string, route_table_t> routes_;
  };

} // namespace schwifty::krabby