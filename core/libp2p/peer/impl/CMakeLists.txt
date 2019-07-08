# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_identity_manager
    identity_manager_impl.hpp
    identity_manager_impl.cpp
    )
target_link_libraries(libp2p_identity_manager
    libp2p_peer_id
    )
