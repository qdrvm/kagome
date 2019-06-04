#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(plaintext_adaptor_test
    plaintext_adaptor_test.cpp
    )
target_link_libraries(plaintext_adaptor_test
    security_plaintext
    buffer
    libp2p_peer_id
    multihash
    multiaddress
    )
