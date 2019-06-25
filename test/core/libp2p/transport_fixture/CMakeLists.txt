#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(transport_test_fixture
    transport_fixture.cpp
    transport_fixture.hpp
    )
target_link_libraries(transport_test_fixture
    multiaddress
    tcp_transport
    GTest::main
    GMock::main
    )
