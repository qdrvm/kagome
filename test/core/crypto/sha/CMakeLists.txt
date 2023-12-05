#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(sha256_test
    sha256_test.cpp
    )
target_link_libraries(sha256_test
    sha
    )
