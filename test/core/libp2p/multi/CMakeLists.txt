#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(multiaddress_test
    multiaddress_test.cpp
    )
target_link_libraries(multiaddress_test
    multiaddress
    buffer
    )

addtest(multibase_test
    multibase_test.cpp
    )
target_link_libraries(multibase_test
    multibase
    )
