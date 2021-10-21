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
    uri.cpp
    http_request.cpp
    )
target_link_libraries(http_request
    Boost::boost
    crypto
    ssl
    buffer
    )

add_library(offchain_worker
    offchain_worker_impl.cpp
    )
target_link_libraries(offchain_worker
    http_request
    )

add_library(offchain_worker_factory
    offchain_worker_factory_impl.cpp
    )
target_link_libraries(offchain_worker_factory
    offchain_worker
    )


