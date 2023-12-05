#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(basic_code_provider
    common/basic_code_provider.cpp
    )
target_link_libraries(basic_code_provider
    filesystem
    )
