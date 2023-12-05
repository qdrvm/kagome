[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Guide for `outcome::result<T>`

Use `outcome::result<T>` from `<outcome/outcome.hpp>` to represent either value of type `T` or `std::error_code`.

DO NOT DEFINE CUSTOM ERROR TYPES. There is one good explanation for that -- one can not merge two custom types automatically, however error codes can be merged.

Please, read https://ned14.github.io/outcome/ carefully.

## Creating `outcome::result<T>`

#### **Example 1** - Enum in namespace:
```C++
/////////////
// mylib.hpp:

#include <outcome/outcome.hpp>

namespace my::super:lib {
  enum class MyError {
    Case1 = 1,  // NOTE: MUST NOT start with 0 (it represents success)
    Case2 = 2,
    Case3 = 4,  // any codes may be used
    Case4       // or no codes at all
  };

  outcome::result<int> calc(int a, int b);
}
// declare required functions in hpp
// outside of any namespace
OUTCOME_HPP_DECLARE_ERROR(my::super::lib, MyError);


/////////////
// mylib.cpp:
#include "mylib.hpp"

// outside of any namespace
OUTCOME_CPP_DEFINE_CATEGORY(my::super::lib, MyError, e){
  using my::super::lib::MyError; // not necessary, just for convenience
  switch(e) {
    case MyError::Case1: return "Case1 message";
    case MyError::Case2: return "Case2 message";
    case MyError::Case3: return "Case3 message";
    case MyError::Case4: return "Case4 message";
    default: return "unknown"; // NOTE: do not forget to handle everything else
  }
}

namespace my::super::lib {
  outcome::result<int> calc(int a, int b){
    // then simply return enum in case of error:
    if(a < 0)   return MyError::Case1;
    if(a > 100) return MyError::Case2;
    if(b < 0)   return MyError::Case3;
    if(b < 100) return MyError::Case4;

    return a + b; // simply return value in case of value:
  }
}
```


#### **Example 2** - Enum as class member:

```diff
/////////////
// mylib.hpp:

#include <outcome/outcome.hpp>

namespace my::super:lib {

  class MyLib {
   public:
    // MyError now is a member of class
+   enum class MyError {
+     Case1 = 1,  // NOTE: MUST NOT start with 0 (it represents success)
+     Case2 = 2,
+     Case3 = 4,  // any codes may be used
+     Case4       // or no codes at all
+   };

    outcome::result<int> calc(int a, int b);
  }
}
// declare required functions in hpp
// outside of any namespace
+// NOTE: 1 args is only namespace, class prefix should be added to enum
-OUTCOME_HPP_DECLARE_ERROR(my::super::lib, MyError);
+OUTCOME_HPP_DECLARE_ERROR(my::super::lib, MyLib::MyError);

/////////////
// mylib.cpp:
#include "mylib.hpp"

-OUTCOME_CPP_DEFINE_CATEGORY(my::super::lib, MyError, e){
+OUTCOME_CPP_DEFINE_CATEGORY(my::super::lib, MyLib::MyError, e){
- using my::super::lib::MyError; // not necessary, just for convenience
+ using my::super::lib::MyLib::MyError; // not necessary, just for convenience
  switch(e) {
    case MyError::Case1: return "Case1 message";
    case MyError::Case2: return "Case2 message";
    case MyError::Case3: return "Case3 message";
    case MyError::Case4: return "Case4 message";
    default: return "unknown"; // NOTE: do not forget to handle everything else
  }
}

namespace my::super::lib {
-  outcome::result<int> calc(int a, int b)
+  outcome::result<int> MyLib::calc(int a, int b){
    // then simply return enum in case of error:
    if(a < 0)   return MyError::Case1;
    if(a > 100) return MyError::Case2;
    if(b < 0)   return MyError::Case3;
    if(b > 100) return MyError::Case4;

    return a + b; // simply return value in case of value
  }
}
```


## Inspecting `outcome::result<T>`

Inspecting is very straightforward:

```C++

outcome::result<int> calc(int a, int b){
  // then simply return enum in case of error:
  if(a < 0)   return MyError::Case1;
  if(a > 100) return MyError::Case2;
  if(b < 0)   return MyError::Case3;
  if(b < 100) return MyError::Case4;

  return a + b; // simply return value in case of value:
}

outcome::result<int> parent(int a) {
  // NOTE: returns error if calc returned it, otherwise get unwrapped calc result
  OUTCOME_TRY(val, calc(a, 1);  // use convenient macro
  // here val=a+1

  // or

  auto&& result = calc(a, 2);
  if(result) {
    // has value
    auto&& v = result.value();
    return v;
  } else {
    // has error
    auto&& e = result.error(); // get std::error_code
    return e;
  }

  // or

  // pass result to parent
  return calc(a, 3);
}
```
