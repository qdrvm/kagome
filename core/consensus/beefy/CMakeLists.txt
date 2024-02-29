#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(beefy
    impl/beefy_impl.cpp
)
target_link_libraries(beefy
    consensus_common
)
kagome_install(beefy)
kagome_clear_objects(beefy)
