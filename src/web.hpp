#pragma once

#include <string>
#include <vector>

std::string query_string_from_params(std::vector<std::pair<std::string, std::string>> const& params);