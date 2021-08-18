#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(host_api
    impl/host_api_impl.cpp
    )
target_link_libraries(host_api
    crypto_extension
    io_extension
    memory_extension
    misc_extension
    storage_extension
    )
kagome_install(host_api)
