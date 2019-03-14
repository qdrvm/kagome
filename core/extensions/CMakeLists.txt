#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(extensions
    extension_impl.cpp
    impl/crypto_extension.cpp
    impl/io_extension.cpp
    impl/memory_extension.cpp
    impl/misc_extension.cpp
    impl/sandboxing_extension.cpp
    impl/storage_extension.cpp
    impl/memory.hpp
    impl/memory.cpp
    impl/shell_external_interface.hpp
    impl/shell_external_interface.cpp
    )

target_link_libraries(extensions
    binaryen::binaryen
)
