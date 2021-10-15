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

add_library(http_request
    http_request.cpp
    )
target_link_libraries(http_request
    Boost::boost
    )

add_library(offchain_worker
    offchain_worker_impl.cpp
    )
target_link_libraries(offchain_worker
    http_request
    )


