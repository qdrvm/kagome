#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(address_publisher
        address_publisher_impl.cpp
        )
target_link_libraries(address_publisher
        logger
        authority_discovery_proto
        keys_proto
        )
