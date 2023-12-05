#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

add_library(address_publisher
    publisher/address_publisher.cpp
    query/query_impl.cpp
    )
target_link_libraries(address_publisher
    authority_discovery_proto
    logger
    sha
    )
