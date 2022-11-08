#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(validator_parachain
    availability/bitfield/signer.cpp
    availability/bitfield/store_impl.cpp
    availability/store/store_impl.cpp
    pvf/pvf_impl.cpp
    validator/impl/parachain_observer.cpp
    validator/impl/parachain_processor.cpp
    validator/signer.cpp
    approval/approval.cpp)

target_link_libraries(validator_parachain
    crypto_store
    network
    )
