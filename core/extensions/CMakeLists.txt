#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(extensions
    extension_impl.cpp
    )
target_link_libraries(extensions
    crypto_extension
    io_extension
    memory_extension
    misc_extension
    sandboxing_extension
    storage_extension
    )
target_link_libraries(extensions
    crypto_extension
    io_extension
    memory_extension
    misc_extension
    sandboxing_extension
    storage_extension
    )
