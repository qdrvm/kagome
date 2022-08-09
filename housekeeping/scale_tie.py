#!/usr/bin/env python3

import sys

N = int(sys.argv[1])
S = ""

S += "  template <typename T, size_t N = std::remove_reference_t<T>::scale_tie>\n"
S += "  auto as_tie(T &&v) {\n"
S += "    if constexpr (N == 0) {\n"
S += "      // can't create zero sized binding to check zero size\n"
S += "      return std::tie();\n"
for n in range(1, N + 1):
  S += "    } else if constexpr (N == %d) {\n" % n
  vs = ", ".join("v%d" % i for i in range(n))
  S += "      auto &[%s] = v;\n" % vs
  S += "      return std::tie(%s);\n" % vs
S += "    } else {\n"
S += "      // generate code for bigger tuples\n"
S += "      (void)0;\n"
S += "    }\n"
S += "  }\n"

print(S)
