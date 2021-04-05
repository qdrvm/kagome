#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(configuration_storage_test
    configuration_storage_test.cpp
    )
target_link_libraries(configuration_storage_test
    configuration_storage
    logger_for_tests
    )

addtest(app_state_manager_test
    app_state_manager_test.cpp
    )
target_link_libraries(app_state_manager_test
    app_state_manager
    logger_for_tests
    )

addtest(app_config_test
    app_config_test.cpp
    )
target_link_libraries(app_config_test
    app_config_impl
    Boost::filesystem
    logger_for_tests
    )
