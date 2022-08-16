#!/usr/bin/env python3

import sys

N = int(sys.argv[1])
S = ""

S += "  template <typename T, typename F, size_t N = std::remove_reference_t<T>::scale_tie>\n"
S += "  auto as_tie(T &&v, F &&f) {\n"
for n in range(1, N + 1):
  S += "    %sif constexpr (N == %d) {\n" % ("" if n == 1 else "} else ", n)
  vs = ", ".join("v%d" % i for i in range(n))
  S += "      auto &[%s] = v;\n" % vs
  S += "      return std::forward<F>(f)(std::tie(%s));\n" % vs
S += "    } else {\n"
S += "      // generate code for bigger tuples\n"
S += "      static_assert(1 <= N && N <= %d);\n" % N
S += "    }\n"
S += "  }\n"

print(S)
