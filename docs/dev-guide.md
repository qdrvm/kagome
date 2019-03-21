# Development guide

We, at Kagome, enforce [**clean architecture**](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html) as much as possible.

Currently we use only two layers:

## *an entity layer*

- Entity represents domain object. Example: PeerId, Address, Customer, Socket.
- **MUST** store state (data members). Has interface methods to work with this state.
- **MAY** be represented as plain struct or complex class (developer is free to choose either)
- **MUST NOT** depend on *services*
- **CAN NOT** be mocked in other services, when writing tests
- if it happens that you want to add some functionality to Entity, but functionality depends on a Service, then create new Service, which will work with this Entity:

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

  // 3. instead, create an interface for "service Hasher":
  class Hasher {
    public:
      virtual ~Hasher() = default;
      virtual Hash256 sha2_256(const Block&b) const = 0;
  };

  // 4. with implementation
  class HasherImpl: public Hasher {
    public:
      Hash256 sha2_256(const Block&b) const {
        ... an actual implementation of hash() ...
      }
  };

  ```
  Rationale -- services can be easily mocked in tests. With the Hasher example, we can create block with predefined hash.

## *a service layer*

- Service represents any piece of functionality
- **MAY** store state (data members)
- **MUST** have base class-interface with public virtual default destructor

  Example:
  ```C++
  // hasher.hpp:
  class HasherService {
    public:
     virtual ~HasherService() = default;

     virtual Hash256 sha2_256(Buffer b) const = 0;
     ...
  }

  // impl/hasher.hpp:
  class HasherServiceImpl : public Hasher {
    public:
      // HasherService depends on SomeOtherService, so we pass it
      // through constructor as dependency
      HasherServiceImpl(const SomeOtherService &dep);
      Hash256 sha2_256(Buffer b) const override;
      ...

    private:
      const SomeOtherService& dep_;
  };

  // impl/hasher.cpp:
  Hash256 HasherServiceImpl::sha2_256(Buffer b) const {
    ... an actual implementation...
  }
  ```
- **MUST** accept *other services* in constructor of implementation class
- **CAN** be mocked in other services, when writing tests.


This policy ensures testability and maintainability for projects of any size, from 1KLOC to 10MLOC.
