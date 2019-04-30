#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(primitives_test
        primitives_codec_test.cpp
        )

target_link_libraries(primitives_test
        outcome
        scale_codec
        buffer
        scale
        Boost::boost
        )
