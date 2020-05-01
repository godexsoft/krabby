#include "database.hpp"
#include "types.hpp"

#include <sqlcppbridge.h>

// ---- common ----
DEFINE_SQL_TRIVIAL_TABLE(flags, std::vector<bool>);
DEFINE_SQL_TRIVIAL_TABLE(str_str_kv, schwifty::krabby::kv_map_t);

// ---- database definition ----
DEFINE_SQL_DATABASE(krabby, 1, schwifty::krabby::kv_map_t)::upgrade_structure(size_t from, size_t to){
    // if (from<=2 && to>2)
    //    execute("CREATE INDEX IF NOT EXISTS BLAH_BLAH_INDEX ON MY_TABLE (BLAH_BLAH_ID)");
};

namespace schwifty::krabby {

}  // namespace schwifty::krabby