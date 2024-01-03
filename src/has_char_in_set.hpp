#pragma once

#include <bit>
#include <string>

inline uint8_t haschar_char_to_uint8(const char ch) {
    return *((const uint8_t*)(const void*)(&ch));
}

struct haschar_charset {
    bool charset[256];

    inline void reset(std::string const& tuple) {
        for (uint16_t i = 0; i < 256; ++i) {
            charset[i] = false;
        }

        for (uint16_t i = 0, len = tuple.size(); i < len; ++i) {
            charset[haschar_char_to_uint8(tuple[i])] = true;
        }
    }

    inline bool has(const char ch) const {
        return charset[haschar_char_to_uint8(ch)];
    }

    inline haschar_charset(std::string const& tuple) {
        this->reset(tuple);
    }
};

// benchmark:

// #include <bitset>

// typedef bool FindUtil[256];
// typedef std::bitset<256> FindUtil2;

// inline uint8_t char_to_uint8(const char ch) {
//     return *((uint8_t*)(void*)(&ch));
// }


// inline bool is_in_fu_map(const char ch, FindUtil const& fu) {
//   return fu[char_to_uint8(ch)];
// }

// void init_fu_map(std::string const& tuple, FindUtil* fu) {
//   // memset((void*)fu, 0, sizeof(FindUtil));
//   for (size_t i = 0; i < 256; ++i) {
//     (*fu)[i] = false;
//   }

//   for (size_t i = 0; i < tuple.size(); ++i) {
//     (*fu)[char_to_uint8(tuple[i])] = true;
//   }
// }

// inline bool is_in_fu_map2(const char ch, FindUtil2 const& fu) {
//   return fu[char_to_uint8(ch)];
// }

// void init_fu_map2(std::string const& tuple, FindUtil2* fu) {
//   fu->reset();

//   for (size_t i = 0; i < tuple.size(); ++i) {
//     fu->set(char_to_uint8(tuple[i]), true);
//   }
// }

// static void UseFuMap1(benchmark::State& state) {
//   FindUtil fu;
//   init_fu_map("[]:;)(", &fu);

//   for (auto _ : state) {
//     bool status = is_in_fu_map(')', fu);
//     benchmark::DoNotOptimize(status);
//   }
// }
// BENCHMARK(UseFuMap1);

// static void DirectFindInStr(benchmark::State& state) {
//   std::string map = "[]:;)(";
  
//   for (auto _ : state) {
//     bool status = map.find(')') != std::string::npos;
//     benchmark::DoNotOptimize(status);
//   }
// }
// // Register the function as a benchmark
// BENCHMARK(DirectFindInStr);

// static void DirectFindInStr2(benchmark::State& state) {
//   const char* map = "[]:;)(";
//   const char* map_end = &map[5];
  
//   for (auto _ : state) {
//     const char* it = map;
//     bool status = false;
//     while (it != map_end) {
//       if (*it == ')') {
//         status = true;
//         break;
//       }
//       ++it;
//     }
//     benchmark::DoNotOptimize(status);
//   }
// }
// // Register the function as a benchmark
// BENCHMARK(DirectFindInStr2);

// static void UseFuMap2(benchmark::State& state) {
//   FindUtil2 fu;
//   init_fu_map2("[]:;)(", &fu);

//   for (auto _ : state) {
//     bool status = is_in_fu_map2(')', fu);
//     benchmark::DoNotOptimize(status);
//   }
// }
// BENCHMARK(UseFuMap2);