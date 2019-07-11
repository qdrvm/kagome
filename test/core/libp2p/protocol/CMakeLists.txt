# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(echo_test echo_test.cpp)
target_link_libraries(echo_test
    libp2p_protocol_echo
    )
