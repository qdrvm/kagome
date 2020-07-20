#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(authorities_manager_error
    authority_manager_error.cpp
    authority_manager_error.hpp
    )
target_link_libraries(authorities_manager_error
    outcome
    )
