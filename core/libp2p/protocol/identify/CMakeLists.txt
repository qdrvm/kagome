#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(pb)

add_library(libp2p_identify
    identify.cpp
    identify.hpp
    observed_addresses.cpp
    observed_addresses.hpp
    )
target_link_libraries(libp2p_identify
    libp2p
    identify_proto
    protobuf_message_read_writer
    )
