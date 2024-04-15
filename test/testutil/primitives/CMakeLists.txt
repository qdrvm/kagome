#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(testutil_primitives_generator
    mp_utils.hpp
    hash_creator.cpp
    )
target_link_libraries(testutil_primitives_generator
    blob
    )
