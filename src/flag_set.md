```cpp

DECL_FLAG_SET(Flags, uint8_t,
    flag_a,
    flag_b,
    flag_c
);

Flags flags = { Flags::_FLAG_flag_a | Flags::_FLAG_flag_b };
flags.set_flag_a(false);

flags.flags & Flags::_FLAG_flag_a == false;

flags.has_flag_b() == true;

flags.debug_print(std::cout); // [flag_b, ]

```
