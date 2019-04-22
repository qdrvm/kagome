##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

addtest(byte_array_stream_test
    byte_array_stream_test.cpp
    )
target_link_libraries(byte_array_stream_test
    scale
    )

addtest(scale_compact_test
    scale_compact_test.cpp
    )
target_link_libraries(scale_compact_test
    scale
    buffer
    )

addtest(scale_fixed_test
    scale_fixed_test.cpp
    )
target_link_libraries(scale_fixed_test
    scale
    buffer
    )

addtest(scale_boolean_test
    scale_boolean_test.cpp
    )
target_link_libraries(scale_boolean_test
    scale
    buffer
    )

addtest(scale_optional_test
    scale_optional_test.cpp
    )
target_link_libraries(scale_optional_test
    scale
    buffer
    )

addtest(scale_collection_test
    scale_collection_test.cpp
    )
target_link_libraries(scale_collection_test
    scale
    buffer
    )

addtest(scale_variant_test
    scale_variant_test.cpp
    )
target_link_libraries(scale_variant_test
    scale
    buffer
    )

addtest(scale_pair_test
    scale_pair_test.cpp
    )
target_link_libraries(scale_pair_test
    scale
    buffer
    )
