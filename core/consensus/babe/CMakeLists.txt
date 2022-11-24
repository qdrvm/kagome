#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(babe_error
    babe_error.cpp
    babe_error.hpp
    )
target_link_libraries(babe_error
    outcome
    )
kagome_install(babe_error)
