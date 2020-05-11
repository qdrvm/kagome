#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(state_api_service
    state_jrpc_processor.cpp
    impl/state_api_impl.cpp
    impl/state_jrpc_param_parser.cpp
    )
target_link_libraries(state_api_service
    buffer
    api_service
    trie_storage
    )
