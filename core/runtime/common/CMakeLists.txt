#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#


add_library(storage_wasm_provider
    storage_wasm_provider.cpp
    )
target_link_libraries(storage_wasm_provider
    buffer
    )
