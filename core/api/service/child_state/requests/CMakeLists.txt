#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_child_state_requests
    get_keys.cpp
    get_keys_paged.cpp
    get_storage.cpp
    get_storage_hash.cpp
    get_storage_size.cpp
    )

target_link_libraries(api_child_state_requests
    Boost::boost
    blob
    )
