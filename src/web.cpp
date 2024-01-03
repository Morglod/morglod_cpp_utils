#include "./web.hpp"

#include <cpr/cpr.h>

std::string query_string_from_params(std::vector<std::pair<std::string, std::string>> const& params) {
    std::string query_string = "";
    for (auto const& [ k, v ] : params) {
        query_string += k + "=" + cpr::util::urlEncode(v) + "&";
    }
    return query_string;
}
