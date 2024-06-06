#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(formatters_test
    formatters_test.cpp
    )
target_link_libraries(formatters_test
    outcome
    soralog::soralog
    blob
    )

addtest(di_test di_test.cpp)


add_subdirectory(rocksdb)
