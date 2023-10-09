#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(keccak_test
    keccak_test.cpp
    )
target_link_libraries(keccak_test
    keccak
    hexutil
    )
