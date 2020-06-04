#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_state_requests
    get_storage.cpp
    )

target_link_libraries(api_state_requests
    Boost::boost
    blob
    )
