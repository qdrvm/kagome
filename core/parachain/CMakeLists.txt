#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(validator_parachain
    availability/bitfield/signer.cpp
    availability/bitfield/store_impl.cpp
    availability/erasure_coding_error.cpp
    availability/fetch/fetch_impl.cpp
    availability/recovery/recovery_impl.cpp
    availability/store/store_impl.cpp
    backing/store_impl.cpp
    pvf/precheck.cpp
    pvf/pvf_impl.cpp
    pvf/pvf_runtime_cache.cpp
    validator/impl/parachain_observer_impl.cpp
    validator/impl/parachain_processor.cpp
    validator/signer.cpp
    approval/approval_distribution.cpp
    approval/approval.cpp
    backing/store_impl.cpp
    )

target_link_libraries(validator_parachain
    crypto_store
    network
    erasure_coding_crust::ec-cpp
    )
