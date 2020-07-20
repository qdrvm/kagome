#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(authority_manager
    authority_manager_impl.cpp
    authority_manager_impl.hpp
    )
target_link_libraries(authority_manager
    logger
    authorities_manager_error
    )
