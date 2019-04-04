#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(uvarint_test
    uvarint_test.cpp
    )

target_link_libraries(uvarint_test
    uvarint
    hexutil
    )

addtest(protocol_list_test
    protocol_list_test.cpp
    )
target_link_libraries(protocol_list_test
    protocol_list
    )
