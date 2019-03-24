#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(primitives_test
        primitives_codec_test.cpp
        )

target_link_libraries(primitives_test
        outcome
        primitives
        buffer
        scale
        Boost::boost
        )
