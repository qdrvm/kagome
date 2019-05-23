# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(outcome)
add_subdirectory(sr25519-crust)

########
# ufiber
add_library(ufiber INTERFACE)
find_package(Threads REQUIRED)
target_include_directories(ufiber INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/ufiber/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ufiber INTERFACE
    Boost::boost
    Boost::context
    Threads::Threads
    )
########
