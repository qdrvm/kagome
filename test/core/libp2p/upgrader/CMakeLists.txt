#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(connection_upgrader_test
    connection_upgrader_test.cpp
    )
target_link_libraries(connection_upgrader_test
    connection_upgrader
    multiaddress
    libp2p_transport
    )
