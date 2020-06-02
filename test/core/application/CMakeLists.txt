#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(configuration_storage_test
    configuration_storage_test.cpp
    )
target_link_libraries(configuration_storage_test
    configuration_storage
    )

addtest(local_key_storage_test
    local_key_storage_test.cpp
    )
target_link_libraries(local_key_storage_test
    local_key_storage
    )

addtest(app_state_manager_test
    app_state_manager_test.cpp
    )
target_link_libraries(app_state_manager_test
    app_state_manager
    )
