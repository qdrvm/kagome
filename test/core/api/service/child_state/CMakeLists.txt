#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(child_state_api_test
    child_state_api_test.cpp
    )
target_link_libraries(child_state_api_test
    api
    blob
    )
