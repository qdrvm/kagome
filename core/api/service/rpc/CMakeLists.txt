#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(rpc_api_service
    rpc_jrpc_processor.cpp
    impl/rpc_api_impl.cpp
    )
target_link_libraries(rpc_api_service
    api_rpc_requests
    api_service
    )

add_subdirectory(requests)
