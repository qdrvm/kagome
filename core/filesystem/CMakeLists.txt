#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(filesystem filesystem.cpp)
target_link_libraries(filesystem Boost::filesystem)
kagome_install(filesystem)
