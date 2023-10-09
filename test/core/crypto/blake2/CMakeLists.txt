#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(blake2_test
    blake2_test.cpp
    )
target_link_libraries(blake2_test
    blake2
    hexutil
    )
