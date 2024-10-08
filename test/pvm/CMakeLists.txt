#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(pvm_test
    pvm_test.cpp
    )

target_link_libraries(pvm_test
    pvm
    )
