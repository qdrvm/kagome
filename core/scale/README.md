# SCALE codec C++ implementation
fully meets polkadot specification.\
It allows encoding and decoding following data types:
* Built-in integer types specified by size:
    * ```uint8_t```, ```int8_t```
    * ```uint16_t```, ```int16_t```
    * ```uint32_t```, ```int32_t```
    * ```uint64_t```, ```int64_t```
* bool values
* pairs of types represented by ```std::pair<T1, T2>```
* compact integers represented by CompactInteger type
* optional values represented by ```std::optional<T>```
    * as special case of optional values ```std::optional<bool>``` is encoded using one byte following specification.
* collections of items represented by ```std::vector<T>```
* variants represented by ```boost::variant<T...>```

## ScaleEncoderStream
class ScaleEncoderStream is in charge of encoding data

```c++
ScaleEncoderStream s;
uint32_t ui32 = 123u;
uint8_t ui8 = 234u;
std::string str = "asdasdasd";
auto * raw_str = "zxczxczx";
bool b = true;
CompactInteger ci = 123456789;
boost::variant<uint8_t, uint32_t, CompactInteger> vint = CompactInteger(12345);
std::optional<std::string> opt_str = "asdfghjkl";
std::optional<bool> opt_bool = false;
std::pair<uint8_t, uint32_t> pair{1u, 2u};
std::vector<uint32_t> coll_ui32 = {1u, 2u, 3u, 4u};
std::vector<std::string> coll_str = {"asd", "fgh", "jkl"};
std::vector<std::vector<int32_t>> coll_coll_i32 = {{1, 2, 3}, {4, 5, 6, 7}};
try {
  s << ui32 << ui8 << str << raw_str << b << ci << vint;
  s << opt_str << opt_bool << pair << coll_ui32 << coll_str << coll_coll_i32;
} catch (std::runtime_error &e) {
  // handle error
  // for example make and return outcome::result
  return outcome::failure(e.code());
}
```
You can now get encoded data:
```c++
ByteArray data = s.data();
```

## ScaleDecoderStream
class ScaleEncoderStream is in charge of encoding data

```c++
ByteArray bytes = {...};
ScaleEncoderStream s(bytes);
uint32_t ui32 = 0u;
uint8_t ui8 = 0u;
std::string str;
bool b = true;
CompactInteger ci;
boost::variant<uint8_t, uint32_t, CompactInteger> vint;
std::optional<std::string> opt_str;
std::optional<bool> opt_bool;
std::pair<uint8_t, uint32_t> pair{};
std::vector<uint32_t> coll_ui32;
std::vector<std::string> coll_str;
std::vector<std::vector<int32_t>> coll_coll_i32;
try {
  s >> ui32 >> ui8 >> str >> b >> ci >> vint;
  s >> opt_str >> opt_bool >> pair >> coll_ui32 >> coll_str >> coll_coll_i32;
} catch (std::system_error &e) {
  // handle error
}
```

## Custom types
You may need to encode or decode custom data types, you have to define custom << and >> operators.
Please note, that your custom data types must be default-constructible.
```c++
struct MyType {
    int a = 0;
    std::string b;
};

ScaleEncoderStream &operator<<(ScaleEncoderStream &s, const MyType &v) {
  return s << v.a << v.b;
}

ScaleDecoderStream &operator>>(ScaleDecoderStream &s, MyType &v) {
  return s >> v.a >> v.b;
}
```
Now you can use them in collections, optionals and variants
```c++
std::vector<MyType> v = {{1, "asd"}, {2, "qwe"}};
ScaleEncoderStream s;
try { 
  s << v;
} catch (...) {
  // handle error
}
```
The same for ```ScaleDecoderStream```
```c++
ByteArray data = {...};
std::vector<MyType> v;
ScaleDecoderStream s{data};
try {
  s >> v;
} catch (...) {
  // handle error
}
```

## Convenience functions
Convenience functions 
```c++
template<class T> 
outcome::result<std::vector<uint8_t>> encode(T &&t);

template <class T>
outcome::result<T> decode(gsl::span<const uint8_t> span)

template <class T>
outcome::result<T> decode(ScaleDecoderStream &s)  
```
are wrappers over ```<<``` and ```>>``` operators described above.

Encoding data using ```encode``` convenience function looks as follows:
```c++
std::vector<uint32_t> v = {1u, 2u, 3u, 4u};
auto &&result = encode(v);
if (!res) {
    // handle error
}
```

Decoding data using ```decode``` convenience function looks as follows:

```c++
ByteArray bytes = {...};
outcome::result<MyType> result = decode<MyType>(bytes);
if (!result) {
    // handle error
}
```
or
```c++
ByteArray bytes = {...};
ScaleDecoderStream s(bytes);
outcome::result<MyType> result = decode<MyType>(s);
if (!result) {
    // handle error
}
```
