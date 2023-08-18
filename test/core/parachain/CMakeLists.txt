#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(assignments_test
    assignments.cpp
    )
target_link_libraries(assignments_test
    crypto_store
    base_fs_test
    validator_parachain
    )
