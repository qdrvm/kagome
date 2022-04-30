#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(offchain_persistent_storage
    offchain_persistent_storage.cpp
    )
target_link_libraries(offchain_persistent_storage
    Boost::boost
    )
kagome_install(offchain_persistent_storage)

add_library(offchain_local_storage
    offchain_local_storage.cpp
    )
target_link_libraries(offchain_local_storage
    Boost::boost
    )
kagome_install(offchain_local_storage)

add_library(http_request
    http_request.cpp
    )
target_link_libraries(http_request
    kagome_uri
    Boost::boost
    OpenSSL::Crypto
    OpenSSL::SSL
    buffer
    )
kagome_install(http_request)

add_library(offchain_worker
    offchain_worker_impl.cpp
    )
target_link_libraries(offchain_worker
    http_request
    )
kagome_install(offchain_worker)

add_library(offchain_worker_factory
    offchain_worker_factory_impl.cpp
    )
target_link_libraries(offchain_worker_factory
    offchain_worker
    )
kagome_install(offchain_worker_factory)

add_library(offchain_worker_pool
    offchain_worker_pool_impl.cpp
    )
target_link_libraries(offchain_worker_pool
    offchain_worker
    )
kagome_install(offchain_worker_pool)

