#include "./hmac_sha256.hpp"

#include "fmt/core.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>

// https://stackoverflow.com/questions/62458175/hmac-sha256-in-c-with-openssl-how-to-get-a-correct-output

std::string hmac_sha256(
    std::string const& key,
    const void *data, size_t const& datalen
) {
    unsigned char result[1024];
    unsigned int out_len = 0;

    HMAC(EVP_sha256(), key.c_str(), (int)key.size(), (const unsigned char*)data, datalen, &result[0], &out_len);

    // to hex
    std::string hexed;
    hexed.reserve(out_len * 2);

    for (size_t i = 0; i < out_len; ++i) {
        std::string h = fmt::format("{:x}", (int)result[i]);
        if (h.size() == 1) hexed += '0';
        hexed += h;
    }

    hexed.shrink_to_fit();

    return hexed;

    // test
    // in symbol=LTCBTC&side=BUY&type=LIMIT&timeInForce=GTC&quantity=1&price=0.1&recvWindow=5000&timestamp=1499827319559
    // secret NhqPtmdSJYdKjVHjA7PZj4Mge3R5YNiP1e3UZjInClVN65XAbvqqM6A7H5fATj0j
    // out c8db56825ae71d6d79447849e617115f4a920fa2acdcab2b053c4b2838bd6b71
}
