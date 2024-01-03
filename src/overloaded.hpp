#pragma once

// refs:
// https://dev.to/tmr232/that-overloaded-trick-overloading-lambdas-in-c17
// https://arne-mertz.de/2017/06/class-template-argument-deduction/

// Example usage

// std::variant<int, float> my_val;

// std::visit(overloaded {
//     [](int x) {
//         printf("%i", x);
//     },
//     [](float y) {
//         printf("%f", y);
//     }
// }, my_val);


// void foo() {
//     auto print_me = overloaded {
//         [](int x) {
//             printf("%i", x);
//         },
//         [](std::string s) {
//             printf("%s", s.c_str());
//         }
//     };

//     print_me(10);
//     print_me("hello");
// }

// kind of same as V but for lambdas

// void foo() {
//     struct print_me_t {
//         void operator () (int x) {
//             printf("%i", x);
//         }
//         void operator () (std::string s) {
//             printf("%s", s.c_str());
//         }
//     };
//     print_me_t print_me;

//     print_me(10);
//     print_me("hello");
// }

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

// deduction guidline
template <class...Ts>
overloaded(Ts...) -> overloaded<Ts...>;
