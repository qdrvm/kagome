#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(storage_explorer
    storage_explorer.cpp
    ${BACKWARD_ENABLE}
    )
target_link_libraries(storage_explorer
    application_injector
    authority_manager
    )
if (BACKWARD)
    add_backward(storage_explorer)
endif()

add_executable(kagome-db-editor
    kagome_db_editor.cpp
    )
target_link_libraries(kagome-db-editor
    Boost::Boost.DI
    block_storage
    hasher
    storage
    runtime_upgrade_tracker
    storage
    )
if (BACKWARD)
  add_backward(kagome-db-editor)
endif()
kagome_install(kagome-db-editor)

