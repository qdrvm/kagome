#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(prospective_parachains
    candidate_storage.cpp
    fragment.cpp
    constraints.cpp
    backed_chain.cpp
    scope.cpp
    fragment_chain.cpp
    prospective_parachains.cpp
    fragment_chain_errors.cpp
    )

target_link_libraries(prospective_parachains
    fmt::fmt
    scale::scale
    soralog::soralog
    logger
    Boost::boost
    outcome
    )

