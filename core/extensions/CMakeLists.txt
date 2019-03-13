#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(extensions
    extension_impl.cpp
    impl/crypto_extension.cpp
    impl/io_extension.cpp
    impl/memory_extension.cpp
    impl/misc_extension.cpp
    impl/sandboxing_extension.cpp
    impl/storage_extension.cpp
    )
target_link_libraries(extensions
    crypto_extension
    io_extension
    memory_extension
    misc_extension
    sandboxing_extension
    storage_extension
    )
