#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(requests)

add_library(chain_api_service
    chain_jrpc_processor.cpp
    impl/chain_api_impl.cpp
    )
target_link_libraries(chain_api_service
    api_service
    api_chain_requests
    hexutil
    )
