#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(extensions
    impl/extension_impl.cpp
    )
target_link_libraries(extensions
    crypto_extension
    io_extension
    memory_extension
    misc_extension
    storage_extension
    )
kagome_install(extensions)

add_library(extension_factory
    impl/extension_factory_impl.cpp
    )
target_link_libraries(extension_factory
    extensions
    )
kagome_install(extension_factory)
