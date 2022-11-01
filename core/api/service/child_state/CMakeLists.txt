#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(child_state_api_service
    child_state_jrpc_processor.cpp
    impl/child_state_api_impl.cpp
    )
target_link_libraries(child_state_api_service
    api_child_state_requests
    api_service
    trie_storage
    blob
    metadata_api
    )

add_subdirectory(requests)
