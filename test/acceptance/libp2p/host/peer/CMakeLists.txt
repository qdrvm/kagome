# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(test_peer
    test_peer.hpp
    test_peer.cpp
    )
target_link_libraries(test_peer
    Boost::boost
    client_test_session
    )
