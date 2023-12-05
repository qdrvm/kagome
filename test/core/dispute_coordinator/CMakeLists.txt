#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(dispute_coordinator_test
    test.cpp
    )
target_link_libraries(dispute_coordinator_test
    scale::scale
    blob
    Boost::boost
    )
