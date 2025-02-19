#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(pvm
    sections.cpp
    errors.cpp
    sandbox.cpp
    )

target_link_libraries(pvm
    fmt::fmt
    scale::scale
    soralog::soralog
    logger
    outcome
    )

