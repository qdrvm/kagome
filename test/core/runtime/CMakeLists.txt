#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(wasm_memory_test
    wasm_memory_test.cpp
    )
target_link_libraries(wasm_memory_test
    memory
    )

addtest(wasm_memory_stream_test
    wasm_memory_stream_test.cpp
    )
target_link_libraries(wasm_memory_stream_test
    wasm_memory_stream
    )
