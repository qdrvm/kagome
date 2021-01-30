#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(system_api_service
    system_jrpc_processor.cpp
    impl/system_api_impl.cpp
    )
target_link_libraries(system_api_service
    api_system_requests
    babe
    ss58_codec
    )

add_subdirectory(requests)
