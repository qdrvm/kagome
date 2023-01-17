#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(hexutil
    hexutil.hpp
    hexutil.cpp
    )
target_link_libraries(hexutil
    Boost::boost
    outcome
    )
kagome_install(hexutil)

add_library(blob
    blob.hpp
    blob.cpp
    )
target_link_libraries(blob
    hexutil
    )
kagome_install(blob)

add_library(fd_limit
    fd_limit.hpp
    fd_limit.cpp
    )
target_link_libraries(fd_limit
    Boost::boost
    logger
    )

add_library(mp_utils
        int_serialization.cpp
        int_serialization.hpp
    )
target_link_libraries(mp_utils
    Boost::boost
    )
kagome_install(mp_utils)

add_library(kagome_uri
    uri.hpp
    uri.cpp
    )
kagome_install(kagome_uri)

add_library(spin_lock
    spin_lock.hpp
    spin_lock.cpp
    )
kagome_install(spin_lock)
