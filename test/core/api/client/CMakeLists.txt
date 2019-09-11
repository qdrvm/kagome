#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(test_api_client
    api_client.hpp
    api_client.cpp
    )
target_link_libraries(test_api_client
    Boost::boost
    )
