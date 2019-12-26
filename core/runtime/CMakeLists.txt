#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(common)
add_subdirectory(binaryen)

add_library(memory
    common.hpp
    wasm_result.hpp
    wasm_memory.hpp
    common/wasm_memory_impl.hpp
    common/wasm_memory_impl.cpp
    )
target_link_libraries(memory
    buffer
    )
kagome_install(memory)
