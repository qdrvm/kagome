# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(example_config
    example_config.cpp
    )
target_link_libraries(example_config
    Boost::filesystem
    p2p::p2p_multiaddress
    p2p::p2p_peer_id
    )
