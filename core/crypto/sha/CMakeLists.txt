#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(sha
    sha256.hpp
    sha256.cpp
    )
target_link_libraries(sha
    PUBLIC OpenSSL::SSL
    OpenSSL::Crypto
    blob
    )
kagome_install(sha)
