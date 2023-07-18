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

addtest(pvf_runtime_cache_test
    pvf_runtime_cache_test.cpp
    )

target_link_libraries(pvf_runtime_cache_test
    validator_parachain
    log_configurator
    )
