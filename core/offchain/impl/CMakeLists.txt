#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(offchain_storage
    offchain_storage_impl.cpp
    )
target_link_libraries(offchain_storage
    hasher
    scale
    leveldb
    )
kagome_install(offchain_storage)
