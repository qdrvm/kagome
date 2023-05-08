#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_explorer
    storage_explorer.cpp
    ${BACKWARD_ENABLE}
    )
target_link_libraries(storage_explorer
    application_injector
    consensus
    )
if (BACKWARD)
    add_backward(storage_explorer)
endif()

add_library(kagome-db-editor
    kagome_db_editor.cpp
    )
target_link_libraries(kagome-db-editor
    Boost::Boost.DI
    blockchain
    hasher
    storage
    runtime_upgrade_tracker
    storage
    )
if (BACKWARD)
  add_backward(kagome-db-editor)
endif()

add_library(kagome-benchmark
    benchmark_main.cpp
    )
target_link_libraries(kagome-benchmark
    application_injector
    )