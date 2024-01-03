```cpp

ENUM_STR(MyEnum, uint8_t,
    A,
    B,
    C
)

MyEnum x = MyEnum::A;
x = MyEnum_from_string("A");
auto x_name = MyEnum_to_string(x) == "A";

```
