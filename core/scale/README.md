# SCALE codec C++ implementation
fully meets polkadot specification.\
It allows encoding and decoding following data types:
1. Built-in integer types specified by size:
    * ```uint8_t```, ```int8_t```
    * ```uint16_t```, ```int16_t```
    * ```uint32_t```, ```int32_t```
    * ```uint64_t```, ```int64_t```
2. bool values
3. compact integers represented by CompactInteger type
4. optional values represented by ```std::optional<T>```
    1. as special case of optional values ```std::optional<bool>``` is encoded using one byte as specified in specification.
5. collections of items represented by ```std::vector<T>```
6. variants represented by ```boost::variant<T...>```

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
std::vector<uint32_t> coll_ui32 = {1u, 2u, 3u, 4u};
std::vector<std::string> coll_str = {"asd", "fgh", "jkl"};
std::vector<std::vector<int32_t>> coll_coll_i32 = {{1, 2, 3}, {4, 5, 6, 7}};

OUTCOME_CATCH(s << ui32 << ui8 << str << raw_str << b << ci << vint);
OUTCOME_CATCH(s << opt_str << opt_bool << coll_ui32 << coll_str << coll_coll_i32);
```
You can now get encoded data:
```c++
ByteArray data = s.data();
```
The ```OUTCOME_CATCH``` macro catches exceptions thrown by scale codec and returns them as ```outcome::result```

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

OUTCOME_CATCH(s >> ui32 >> ui8 >> str >> b >> ci >> vint);

std::optional<std::string> opt_str;
std::optional<bool> opt_bool;
std::vector<uint32_t> coll_ui32;
std::vector<std::string> coll_str;
std::vector<std::vector<int32_t>> coll_coll_i32;

OUTCOME_CATCH(s >> opt_str >> opt_bool >> coll_ui32 >> coll_str >> coll_coll_i32);
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
OUTCOME_CATCH(s << v);
```
The same for ```ScaleDecoderStream```
```c++
ByteArray data = {...};
std::vector<MyType> v;
ScaleDecoderStream s{data};
OUTCOME_CATCH(s >> v);
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
