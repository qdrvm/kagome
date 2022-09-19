#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(twox twox.cpp)
target_link_libraries(twox
    xxhash
    blob
  )
kagome_install(twox)
