#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(timeline)
add_subdirectory(babe)
add_subdirectory(grandpa)
#add_subdirectory(sassafras)

add_library(consensus
    validation/babe_block_validator.cpp
    )
target_link_libraries(consensus
    babe
#    sassafras
    grandpa
    )
kagome_install(consensus)
kagome_clear_objects(consensus)
