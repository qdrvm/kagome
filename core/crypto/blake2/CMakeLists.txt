#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(blake2
  blake2s.cpp
  blake2b.cpp
  )
disable_clang_tidy(blake2)
kagome_install(blake2)
