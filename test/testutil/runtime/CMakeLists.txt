# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(basic_wasm_provider
        common/basic_wasm_provider.cpp
    )
target_link_libraries(basic_wasm_provider
    buffer
    Boost::filesystem
    )
