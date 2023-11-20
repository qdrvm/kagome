#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(consensus_common
    block_production_error.cpp
)
target_link_libraries(consensus_common
    logger
    metrics
)
kagome_install(consensus_common)

add_subdirectory(timeline)
add_subdirectory(babe)
add_subdirectory(grandpa)

add_library(consensus
    validation/babe_block_validator.cpp
)
target_link_libraries(consensus
    timeline
    babe
    grandpa
)
kagome_install(consensus)
kagome_clear_objects(consensus)
