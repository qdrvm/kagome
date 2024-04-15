#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(pvf_test
    pvf_test.cpp
    )

addtest(assignments_test
    assignments.cpp
    )
target_link_libraries(assignments_test
    base_fs_test
    )

addtest(prospective_parachains
    prospective_parachains.cpp
    )
