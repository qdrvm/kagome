#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(primitives_codec_test
    primitives_codec_test.cpp
    )

target_link_libraries(primitives_codec_test
    testutil_primitives_generator
    )

addtest(ss58_codec_test
    ss58_codec_test.cpp
    )
