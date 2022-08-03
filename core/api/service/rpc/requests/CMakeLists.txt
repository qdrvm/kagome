#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_rpc_requests
    methods.cpp
    )

target_link_libraries(api_rpc_requests
    Boost::boost
    outcome
    )
