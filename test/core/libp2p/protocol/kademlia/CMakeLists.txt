# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(libp2p_protocol_kademlia_test
    libp2p_protocol_kademlia_test.cpp
    )
target_link_libraries(libp2p_protocol_kademlia_test
    libp2p_protocol_kademlia
    multiaddress
    )
