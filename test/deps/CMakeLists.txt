#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(outcome_test
    outcome_test.cpp
    )
target_link_libraries(outcome_test
    dummy_error
    )

addtest(formatters_test
    formatters_test.cpp
    )

addtest(di_test di_test.cpp)


add_subdirectory(rocksdb)
