[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Rules

We define categories of rules:

- **Design rules** - recommendations that flatly proscribe or require a given practice without exception.
- **Guidelines** - suggested practices of a more abstract nature for which exceptions are sometimes legitimately made.
- **Principles** - certain observations and truths that have often proved useful during the design process but must be evaluated in the context of a specific design.

All of these rules are described in the [^1], readers are welcomed *not to ask rationale here*, but search through the book by themselves for answers.

All used terms are described in [terms.md](./terms.md).

1. **major design rule** - It is almost always an error to place a *definition* with *external linkage* in a `.h` file.

   ```C++
   // radio.h
   #ifndef INCLUDED_RADIO
   #define INCLUDED_RADIO
   int z;                               // illegal: external data definition
   extern int LENGTH = 10;              // illegal: external data definition
   const int WIDTH = 5;                 // avoid: constant data definition
   stati c int y;                       // avoid: static data definition
   static void func() {...}             // avoid: static function definition
   class Radio {
     static int s_count;                // fine: static member declaration
     static const double S__PI;         // fine: static const member dec.
     int d_size;                        // fine: member data definition
   public:
     int size() const;                  // fine: member function declaration
   };                                   // fine: class definition

   inline int Radio::size() const {
     return d_size;
   }                                    // fine: inline function definition

   int Radio::s_count;                  // illegal: static member definition

   double Radio::-S_PI = 3.14159265358; // illegal: static const member def,
   int Radio::size() const { /*...*/ }  // illegal: member function definition
   #endif
   ```

2. **major design rule** - avoid *free functions* with *external linkage*. The *definitions* to be avoided at file scope in `.c` files are data and functions that have *not* been declared static [^1: 1.1.4]

   ```C++
   // file1.c
   int i;                     // avoid: external linkage
   int max(int a, int b){...} // avoid: external linkage
   inline int min(){...}      // fine: internal linkage
   static int mean(){...}     // fine: internal linkage
   class Link;                // fine: internal linkage
   enum {...}                 // fine: internal linkage
   const double PI = 3;       // fine: internal linkage
   static const char *names[] = {"a", "b"} // fine: internal linkage
   typedef struct {..} mytype;// fine: does not introduce new type.
   ```

3. **major design rule** - keep class data members private.

4. **major design rule** - avoid *free functions* (except operator functions) at file scope in `.h`.

5. **major design rule** - avoid  enums, typedefs and constants at file scope in `.h` files.

6. **major design rule** - avoid using preprocessor macros in header files except as include guards.

7. **major design rule** - only classes, structures, unions and free operator functions should be *declared*  at file scope in `.h` file. Only classes, structures , unions, and inline (member or free operator) functions should be *defined* at file scope in a `.h` file.

8. **major design rule** - place a unique and predictable (internal) include guard around the contents of each header file.

9. **guideline** - document the *public  API* so that they are usable by others. Have at least one other developer review each interface.

10. **guideline** - explicitly state conditions under which behavior is undefined.

11. **principle** - the use of `assert` statements can help to document the assumptions you make when implementing your code.

12. **principle** - a component is the appropriate fundamental unit of design.

13. **major design rule** - logical entities *declared* within a component should not be *defined* outside that component.

14. **minor design rule** - the root names of the `.cpp` and the `.hpp` file that comprise a component should match exactly.

15. **major design rule** - the `.cpp` file of every component should include its own `.hpp` file as the first substantive line of code.

16. **guideline** - clients should include header files providing required type definitions directly; except for non-private inheritance, avoid relying on one header file to include another.

17. **major design rule** - avoid definitions with *external linkage* in the `.cpp` file of a component that are not declared explicitly in the corresponding `.hpp` file.

18.  **major design rule** - avoid accessing a *definition* with *external linkage* in another component via a local declaration; instead, include the `.hpp` file for that component.

19. **guideline** - a component `X` should include `y.hpp` only if `X` makes direct substantive use of a class or free operator function defined in `Y`.

20.  **principle** - granting (local) friendship to classes defined within the same component does not violate encapsulation.

    Example: defining an iterator class along with a container class in the same component enables user extensibility, improves maintainability and enhances reusability while preserving encapsulation.

21. **principle** - all tests must be done in  isolation. Testing a component in isolation is an effective way to ensure reliability.

22. **principle** - every directed acyclic graph can be assigned unique level numbers; a graph with cycles cannot. A physical dependency graph that can be assigned unique level numbers is said to be *levelizable*.

23. **principle** - in most real-world situations, large designs must be levelizable if they are able to be tested  effectively.

24. **principle** - testing only the functionality *directly implemented* within a component enables the complexity of the test to be proportional to the complexity of the component.

25. **guideline** - avoid cyclic physical dependencies among dependencies.

26. **principle** - factoring a concrete class into two classes containing higher and lower levels of functionality can facilitate levelization.

27. **principle** - factoring an abstract base class into two classes - one defining a pure interface, the other defining its partial implementation - can facilitate levelization.

28. **principle** - a *protocol class* can be used to eliminate both compile and link time dependencies.

29. **major design rule** - prepend every global identifier with its package prefix.

30. **major design  rule** - avoid cyclic dependencies among packages.

31. **guideline** - avoid declaring results returned by value from functions as `const`.

32. **minor design rule** - never pass a user-defined type to a function by value.

33. **guideline** - avoid using `short` in the interface; use `int` instead.

34. **guideline** - avoid using `unsigned` in the interface; use `int` instead.

35. **guideline** - explicitly declare (public or private) the constructor and assignment operator for any class defined in a header file.

36. **minor design rule** - in every class that declares or is  derived from a class that declares a virtual function, explicitly declare the destructor as the first virtual function in the class and define it out of line.


[^1]: "John Lakos - Large Scale C++ Software Design".
