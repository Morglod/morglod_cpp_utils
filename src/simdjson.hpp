#pragma once

#include "./log.hpp"
#include <simdjson.h>
#include <string>

#ifndef _JSON_DEBUG
#define _JSON_DEBUG 0
#endif

#if _JSON_DEBUG
    #define _JSON_DEBUG_PRINT(_MSG) KS_LOG(_MSG, __FILE__, __LINE__);
#else
    #define _JSON_DEBUG_PRINT(_MSG) 
#endif

#define _JSON_FIELD_BINANCE_ENUM(_FIELD_VALUE, _ENUM, _OUTPUT_FIELD) \
    _JSON_DEBUG_PRINT( "_JSON_FIELD_BINANCE_ENUM " #_ENUM ) \
    _ENUM##_from_string(std::string(std::string_view(_FIELD_VALUE.value())), _OUTPUT_FIELD);

template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, std::string& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value std::string" )
    out = std::string(std::string_view(field.value()));
}
template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, uint8_t& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value uint8_t" )
    out = uint8_t(uint64_t(field.value()));
}
template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, uint32_t& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value uint32_t" )
    out = uint32_t(uint64_t(field.value()));
}
template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, int32_t& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value int32_t" )
    out = int32_t(int64_t(field.value()));
}
template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, uint64_t& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value uint64_t" )
    out = uint64_t(field.value());
}
template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, double& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value double" )
    auto val = field.value();
    simdjson::fallback::ondemand::json_type jt = val.type().value();
    if (jt == simdjson::fallback::ondemand::json_type::string) {
        std::string s;
        _JSON_FIELD_get_value(field, s);
        out = std::stod(s);
    } else {
        out = double(val);
    }
}
template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, float& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value float" )
    double d;
    _JSON_FIELD_get_value(field, d);
    out = float(d);
}
template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, bool& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value bool" )
    out = bool(field.value());
}
// template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, binance::RateLimitType& out) {
//     _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value binance::RateLimitType" )
//     _JSON_FIELD_BINANCE_ENUM(field, binance::RateLimitType, out);
// }
// template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, binance::OrderType& out) {
//     _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value binance::OrderType" )
//     _JSON_FIELD_BINANCE_ENUM(field, binance::OrderType, out);
// }
// template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, binance::OrderStatus& out) {
//     _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value binance::OrderStatus" )
//     _JSON_FIELD_BINANCE_ENUM(field, binance::OrderStatus, out);
// }
// template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, binance::TimeInForce& out) {
//     _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value binance::TimeInForce" )
//     _JSON_FIELD_BINANCE_ENUM(field, binance::TimeInForce, out);
// }
// template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, binance::RateLimitInterval& out) {
//     _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value binance::RateLimitInterval" )
//     _JSON_FIELD_BINANCE_ENUM(field, binance::RateLimitInterval, out);
// }
// template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, SymbolPair& out) {
//     _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value SymbolPair" )
//     out = SymbolPair(std::string(std::string_view(field.value())));
// }
// template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, binance::SymbolStatus& out) {
//     _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value binance::SymbolStatus" )
//     _JSON_FIELD_BINANCE_ENUM(field, binance::SymbolStatus, out);
// }
template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, int64_t& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value binance::SymbolStatus" )
    out = int64_t(field.value());
}
// template<typename FieldT> void _JSON_FIELD_get_value(FieldT& field, SymbolAsset& out) {
//     _JSON_DEBUG_PRINT("_JSON_FIELD_get_value SymbolAsset")
//     out = SymbolAsset(std::string(std::string_view(field.value())));
// }

template<typename FieldT, typename ValueT> void _JSON_FIELD_get_value(FieldT& field, std::optional<ValueT>& out) {
    _JSON_DEBUG_PRINT( "_JSON_FIELD_get_value std::optional< smth >" )
    auto val = field.value();
    simdjson::fallback::ondemand::json_type jt = val.type().value();
    if (jt != simdjson::fallback::ondemand::json_type::null) {
        ValueT result_value;
        _JSON_FIELD_get_value<FieldT>(field, result_value);
    }
}

#define _JSON_ARRAY(_INNER) \
    { \
        _JSON_DEBUG_PRINT( "_JSON_ARRAY" ) \
        auto json_array = json_it.get_array(); \
        for (auto json_it : json_array) { \
            _INNER \
        } \
    }

#define _JSON_OBJECT_DEBUG_PRINT() \
    { _JSON_DEBUG_PRINT( "_JSON_OBJECT_DEBUG_PRINT" ) auto json_obj = json_it.get_object(); for (auto json_it : json_obj) { \
        auto _json_it_value_debug = json_it.value(); \
        KS_LOG("field=", json_it.unescaped_key(), "type=", _json_it_value_debug.type().value(), "value=", _json_it_value_debug.raw_json_token().value()); \
    } \
        json_obj.reset(); \
    }

#define _JSON_OBJECT(_INNER) \
    { _JSON_DEBUG_PRINT( "_JSON_OBJECT" ) auto json_obj = json_it.get_object(); for (auto json_it : json_obj) { \
        const std::string_view json_field_key = json_it.unescaped_key(); \
        _INNER \
    } }

#define _JSON_FIELD_DO(_JSON_FIELD_NAME, _DO) \
    if (json_field_key == _JSON_FIELD_NAME) { \
        _JSON_DEBUG_PRINT( "_JSON_FIELD_DO " _JSON_FIELD_NAME ) \
        auto&& __json_it_ = json_it.value(); \
        auto json_it = __json_it_; \
        _DO \
    }

#define _OR_JSON_FIELD_DO(_JSON_FIELD_NAME, _DO) \
    else _JSON_FIELD_DO(_JSON_FIELD_NAME, _DO)

#define _JSON_FIELD(_JSON_FIELD_NAME, _OUTPUT_FIELD) \
    if (json_field_key == _JSON_FIELD_NAME) { _JSON_DEBUG_PRINT( "_JSON_FIELD " _JSON_FIELD_NAME ) _JSON_FIELD_get_value(json_it, _OUTPUT_FIELD); }

#define _OR_JSON_FIELD(_JSON_FIELD_NAME, _OUTPUT_FIELD) \
    else _JSON_FIELD(_JSON_FIELD_NAME, _OUTPUT_FIELD)
