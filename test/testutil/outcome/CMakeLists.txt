#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(dummy_error
    dummy_error.cpp
    )
target_link_libraries(dummy_error
    outcome
    )
