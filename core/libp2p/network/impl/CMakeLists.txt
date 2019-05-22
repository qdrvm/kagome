#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libp2p_router
    router_impl.hpp
    router_impl.cpp
    )
target_link_libraries(libp2p_router
    tsl_hat_trie
    )
