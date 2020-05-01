#pragma once

#include "singleton.hpp"
#include "types.hpp"
#include <sqlcppbridge.h>

namespace schwifty::krabby
{

  namespace sql = sql_bridge;

  class database
  {
  public:
    database(std::string storage_path)
        : storage_(storage_path)
    {
    }

    inline sql::context ctx() { return storage_["krabby"]; } // default context accessor
    inline sql::context operator[](std::string const &nm) { return storage_[nm]; }

  private:
    sql::local_storage<sql::sqlite_adapter> storage_; // database storage access
  };

} // namespace schwifty::krabby