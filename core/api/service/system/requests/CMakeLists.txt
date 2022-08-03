#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_system_requests
    name.cpp
    version.cpp
    chain.cpp
    chain_type.cpp
    properties.cpp
    health.cpp
    )

target_link_libraries(api_system_requests
    Boost::boost
    scale::scale
    outcome
    )
