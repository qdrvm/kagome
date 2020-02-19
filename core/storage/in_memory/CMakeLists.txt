# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(in_memory_error
    in_memory_error.cpp
    )
target_link_libraries(in_memory_error
    Boost::boost
    outcome
    )

add_library(in_memory_storage
    in_memory_storage.cpp
    )
target_link_libraries(in_memory_storage
    buffer
    in_memory_error
    )
kagome_install(in_memory_error)
kagome_install(in_memory_storage)
