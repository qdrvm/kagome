# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(outcome INTERFACE)
target_link_libraries(outcome INTERFACE
    Boost::boost
    p2p::p2p
    )
kagome_install(outcome)
