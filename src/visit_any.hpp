#pragma once

#include <any>

// visit_any<
//     text,
//     seq,
//     repeat,
//     capture,
//     capture_group,
//     in_range
// >(overloaded {
//     [](text x) {
//         std::cout << "text(" << x.text << ")" << std::endl;
//     },
//     [](seq s) {
//         std::cout << "seq {" << std::endl;
//         for (auto const& x : s.rules) {
//             print(x);
//         }
//         std::cout << "}" << std::endl;
//     },
//     [](repeat x) {
//         std::cout << "repeat(min=" << x.min << ",max=" << x.max << ", {" << std::endl;
//         print(x.rule);
//         std::cout << "})" << std::endl;
//     },
//     [](capture x) {
//         std::cout << "capture(name=" << x.name << ",type=" << x.type << ",has_type=" << x.has_type << ")" << std::endl;
//     },
//     [](capture_group x) {
//         std::cout << "capture_group(name=" << x.name << ", {" << std::endl;
//         print(x.inside);
//         std::cout << "})" << std::endl;
//     },
//     [](in_range x) {
//         std::cout << "in_range(from=" << x.from << ",to=" << x.to << ")" << std::endl;
//     },
// }, rule);

template<typename... Ts, typename F>
bool visit_any(F&& f, std::any x)
{
    auto result = ((x.type() == typeid(Ts) ?
        (std::forward<F>(f)(*std::any_cast<Ts>(&x)), true) : false) || ...);
    return result;
}