#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(vrf_provider_test
    vrf_provider_test.cpp
    )
target_link_libraries(vrf_provider_test
    vrf_provider
    keccak
    )
