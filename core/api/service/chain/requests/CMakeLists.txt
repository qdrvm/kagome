#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_chain_requests
    get_block_hash.cpp
    )
target_link_libraries(api_chain_requests
    Boost::boost
    buffer
    )
