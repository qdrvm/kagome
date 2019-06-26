#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(message_manager_test
    message_manager_test.cpp
    )
target_link_libraries(message_manager_test
    multiselect
    )

addtest(multiselect_test
    multiselect_test.cpp
    )
target_link_libraries(multiselect_test
    multiselect
    libp2p_peer_id
    )
