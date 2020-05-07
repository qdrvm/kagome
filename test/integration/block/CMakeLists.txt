##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

addtest(block_produce_consume_test
    block_produce_consume_test.cpp
    )
target_link_libraries(block_produce_consume_test
    application_injector
#    binaryen_core_api
    basic_wasm_provider
#    extension_factory
    )
