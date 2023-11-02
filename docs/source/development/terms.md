[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Terms

1. **definition** - provides a unique description of an entity (type, instance, function) within a program:
   1. it declares a function without specifying its body
   2. it contains an `extern` specifier and no initializer or function body
   3. it is the declaration of a static class data member within a class definition
   4. it is a class name declaration
   5. it is a `typedef` or `using` declaration
2. **declaration** - introduces a name into a program:
   1. it defines a static class data member
   2. it defines a non-inline member function
3. **free function** - non-member function, which is not *friend*.
4. **translation unit** - single C++ source file after preprocessor finished including all of the header files.
5. **internal linkage** - name has internal linkage if it is local to its *translation unit* and cannot collide with an identical name defined in another translation unit at link time.
6. **external linkage** - name has external linkage if in multi-file program, that name can interact with other translation units at link time.
7. **target** - shared or static library, executable or command, added to CMake with `add_library`, `add_executable`, `add_custom_target`. With `make` executed as `make <target>`.
8. **public API** - is an interface which is programmatically accessible or detectable by a client.
9. **component** - is the smallest unit of physical design. Or, a set of *translation units*, compiled in a single *target* as a single *library* or *executable*.
10. **unit test** - a set of *translation units* intended to test *public API* of a specific *component* (exactly one!).
11. **regression test** - refers to the practice of comparing the results of running a program given a specific input with a fixed set of expected results, in order to verify that the program continues to behave as expected from one version to the next. In other words, tests which persist actoss versions.
12. **integration test** - refers to the practice when group of *components* (or *packages*) is grouped and tested together as a single unit. Example: session manager with postgres database.
13. **white-box testing** - refers to the practice of verifying the expected behavior of a component by exploiting knowledge of its underlying implementation.
14. **black-box testing** - refers to the practice of verifying the expected behavior of a component based solely on its specification (without knowledge of its implementation).
15. **protocol class** - an abstract class is a protocol class if:
    1. it neither contains nor inherits from classes that contain member data, non-virtual functions, or private (protected) members of any kind.
    2. it has a non-inline virtual destructor defined with an empty implementation (default implementation).
    3. all member functions other than the destructor including inherited functions, are declared pure virtual and left undefined.
16. **packages** - is a collection of *components* organized as a physically cohesive unit.
17. **package group** - is a collection of *packages* organized as a physically cohesive unit.
18. **library** - is a collection of *package groups* organized as a physically cohesive unit.

### Levelization

![](https://user-images.githubusercontent.com/1867551/34187766-5dec01f6-e576-11e7-8c85-1016a917d99a.png)

Given notation as above, we can build a physical (files) and logical (entities) dependency graph.

In perfectly testable system in such graphs there are no cycles (in C++ terms - no circular dependencies) and system consists of clearly defined components, which may be combined in packages.

This graph then is topologically sorted -- this helps to visualize and levelize components.

![](https://user-images.githubusercontent.com/1867551/34187756-5178b00e-e576-11e7-8b71-18946b48cd7a.png)
