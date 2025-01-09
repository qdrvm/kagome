#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(changes_tracker_test
    changes_tracker_test.cpp
    )
target_link_libraries(changes_tracker_test
    storage
    logger_for_tests
    )

