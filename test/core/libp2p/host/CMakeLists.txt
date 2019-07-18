# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(basic_host_test
    basic_host_test.cpp
    )
target_link_libraries(basic_host_test
    libp2p_basic_host
    libp2p_peer_id
    multiaddress
    )
