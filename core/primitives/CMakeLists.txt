#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(primitives
    auth_api.hpp
    block.hpp
    block_header.hpp
    block_id.hpp
    common.hpp
    check_inherents_result.hpp
    digest.hpp
    extrinsic.hpp
    inherent_data.cpp
    inherent_data.hpp
    parachain_host.hpp
    scheduled_change.hpp
    transaction_validity.hpp
    version.hpp
    )
target_link_libraries(primitives
    buffer
    scale
    blob
    )
