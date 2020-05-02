#pragma once

#include <crab/crab.hpp>
#include <functional>
#include <memory>
#include <unordered_map>

namespace schwifty::krabby {
using kv_map_t     = std::unordered_map<std::string, std::string>;
using ws_handler_t = std::function<bool(crab::http::Client *who, crab::http::WebMessage &msg)>;
using dc_handler_t = std::function<void(crab::http::Client *who)>;
}  // namespace schwifty::krabby