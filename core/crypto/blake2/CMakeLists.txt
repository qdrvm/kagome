#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(blake2
  blake2s.cpp
  blake2b.cpp
  )
target_link_libraries(blake2 blob)
disable_clang_tidy(blake2)
kagome_install(blake2)
