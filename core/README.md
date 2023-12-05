[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# core

Every module should be in its folder, so when you include them, it will look like

```
core
├── CMakeLists.txt
├── README.md
└── module1
    └── module1.hpp
```


```
#include <module1/module1.hpp>
```
