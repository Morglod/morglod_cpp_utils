#pragma once

#include <string>

// return hexed string
std::string hmac_sha256(
    std::string const& key,
    const void *data, size_t const& datalen
);