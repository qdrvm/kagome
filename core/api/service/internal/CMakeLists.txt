#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(internal_api_service
    internal_jrpc_processor.cpp
    impl/internal_api_impl.cpp
    )
target_link_libraries(internal_api_service
    api_service
    )
