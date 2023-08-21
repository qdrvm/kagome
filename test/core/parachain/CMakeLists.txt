#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(pvf_test
    pvf_test.cpp
    )

target_link_libraries(pvf_test
    validator_parachain
    log_configurator
    )

addtest(assignments_test
    assignments.cpp
    )
target_link_libraries(assignments_test
    crypto_store
    base_fs_test
    validator_parachain
    )
