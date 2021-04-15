# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(logger_for_tests INTERFACE)
target_link_libraries(logger_for_tests INTERFACE
    logger
    log_configurator
    )

add_subdirectory(libp2p)
add_subdirectory(primitives)
add_subdirectory(runtime)
add_subdirectory(storage)
add_subdirectory(outcome)
