[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Development guide

We, at Kagome, enforce [**clean architecture**](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html) as much as possible.

![image](https://user-images.githubusercontent.com/1867551/54831402-f2a86780-4cc2-11e9-9608-15490c1548ab.png)

## *an entity layer*

- Entity represents domain object. Example: PeerId, Address, Customer, Socket.
- **MUST** store state (data members). Has interface methods to work with this state.
- **MAY** be represented as plain struct or complex class (developer is free to choose either).
- **MUST NOT** depend on *interfaces* or *implementations*.

## *an interface layer*

- An Interface layer contains C++ classes, which have only pure virtual functions and destructor.
- Classes from the *interface layer* **MAY** depend on the *entity layer*, not vice versa.
- **MUST** have public virtual or protected non-virtual destructor ([cppcoreguidelines](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rc-dtor-virtual)).

## *an implementation layer*

- Classes in this layer **MUST** implement one or multiple interfaces from *an interface layer*.
- Example: interface of the `map` with `put, get, contains` methods. One class-implementation may use `leveldb` as backend, another class-implementation may use `lmdb` as backend. Then, it is easy to select required implementation at runtime without recompilation. In tests, it is easy to mock the interface.
- **MAY** store state (data members)
- **MUST** have base class-interface with public virtual default destructor


### Example:

If it happens that you want to add some functionality to Entity, but functionality depends on some Interface, then create new Interface, which will work with this Entity:

Example:

```C++
// an Entity
class Block {
public:
  // getters and setters

  // 1. and you want to add hash()
  Buffer hash();  // 2. NEVER DO THIS
private:
  int fieldA;
  string fieldB;
};

// 3. instead, create an interface for "Hasher":
// "Interface" leyer
class Hasher {
public:
  virtual ~Hasher() = default;
  virtual Hash256 sha2_256(const Block&b) const = 0;
};

// 4. with implementation. Actual implementation may use openssl, for instance.
// "Implementation" layer
class HasherImpl: public Hasher {
public:
  Hash256 sha2_256(const Block&b) const {
    ... an actual implementation of hash() ...
  }
};

```

Rationale -- services can be easily mocked in tests. With the Hasher example, we can create block with predefined hash.

This policy ensures testability and maintainability for projects of any size, from 1KLOC to 10MLOC.
