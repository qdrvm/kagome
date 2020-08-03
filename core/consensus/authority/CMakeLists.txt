#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(authority_manager_error
    authority_manager_error.cpp
    )
target_link_libraries(authority_manager_error
    outcome
    )

add_library(authority_update_observer_error
    authority_update_observer_error.cpp
    )
target_link_libraries(authority_update_observer_error
    outcome
    )
