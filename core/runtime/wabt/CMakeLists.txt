#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(wasm_instrument
    instrument.cpp
    stack_limiter.cpp
    )
target_link_libraries(wasm_instrument
    logger
    outcome
    wabt::wabt
    )
kagome_install(wasm_instrument)
