# CONTRIBUTING

1. Kagome uses C++17 as target language, so use appropriate compilers (gcc-8 is preferred)
2. Use `clang-format`
3. Test your code with gtest/gmock.
4. Open PR with base branch = master, fix CI and follow guide in PR template.


## Specifications

### 1. Module

Module can be defined as a "library", which defines strict interface, and provides one or more implementations of this interface.

1. Module can accept other modules in its constructor.
2. Module can be mocked with GMock.


#### Intent:

- decoupling of the interface and implementation, e.g. [inversion of control](https://en.wikipedia.org/wiki/Inversion_of_control)
- open possibility to provide alternative implementation (e.g. "strategy" pattern)
- pass interface as a "dependency" to other modules to facilitate testing - dependency can be mocked



#### Checklist:

- Must have abstract class-interface
  ```
  core/
    connection/
        connection.hpp // module header
        tcp/           // one of implementations; use 'impl' for single implementation
        tcp.cpp
        tcp.hpp      // class TCP: public Connection;
  ```

- [A base class destructor should be either public and virtual, or protected and nonvirtual](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#c35-a-base-class-destructor-should-be-either-public-and-virtual-or-protected-and-nonvirtual)

- If there is exactly ONE possible implementation for module, add `Impl` suffix to class name:

  ```C++
  class PeerFactory;
  class PeerFactoryImpl : public PeerFactory; // and put it in dir named 'impl'
  ```



#### Example:

```C++
// base class
class Connection{
    public:
    // public virtual noexcept destructor with default impl.
    virtual ~Connection() noexcept = default;
}

// specific implementation
class TCP: public Connection;

class Transport {
    // PREFERABLE: polymorphism via const ref
    // 1. if this class exists only when its dependencies exist, pass deps by const ref
    // 2. constref implies, that no nullcheck is required
    // 3. this class does not have any ownership over Connection dependency
    public:
    Transport(const Connection& connection):
        connection_(connection) {}
    private:
        const Connection& connection_;

    // polymorphism via pointers
    public:
    // prefer this option ONLY if this class HAS OWNERSHIP over dependency
    // prefer std::unique_ptr for unique ownership
    // or std::shared_ptr for shared ownership
    Transport(std::shared_ptr<Connection> connection):
        connection_ptr_(std::move(connection)){}
    private:
        std::shared_ptr<Connection> connection_ptr_;
}
```



## 2. Entity

Entity is a class, which does not have abstract base class, and has state. Example of such classes are `Multiaddress`, `Multistream`, `PeerId`, etc.

1. This type of class SHOULD NOT accept modules in constructor.
2. Entities are not mocked, instead - modules which work with entities mocked.
3. If Entity depends on `Module A`, then **create another module** `Module B`, which can accept `Module A` in constructor and will work with this class-object.
