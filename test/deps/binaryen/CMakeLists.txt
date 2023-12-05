#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(binaryen_test binaryen_test.cpp)
target_link_libraries(binaryen_test
    binaryen::binaryen
    )
