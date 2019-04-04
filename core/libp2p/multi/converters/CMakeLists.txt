#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(converter_utils
    converter_utils.cpp
    converter_utils.hpp
    )

target_link_libraries(converter_utils
    outcome
    buffer
    ip_v4_converter
    ipfs_converter
    udp_converter
    tcp_converter
    )

add_library(ip_v4_converter
    ip_v4_converter.hpp
    ip_v4_converter.cpp)

target_link_libraries(ip_v4_converter
    ip_v4_validator
    )

add_library(tcp_converter
    tcp_converter.hpp
    tcp_converter.cpp
    )

target_link_libraries(tcp_converter
    outcome
    )

add_library(udp_converter
    udp_converter.hpp
    udp_converter.cpp
    )

target_link_libraries(udp_converter
    outcome
    buffer
    )

add_library(ipfs_converter
    ipfs_converter.hpp
    ipfs_converter.cpp
    )

target_link_libraries(ipfs_converter
    uvarint
    base58_codec
    outcome
    )
