##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

addtest(scale_decoder_stream_test
    scale_decoder_stream_test.cpp
    )
target_link_libraries(scale_decoder_stream_test
    scale
    )

addtest(scale_encode_append_test
    scale_encode_append_test.cpp
    )
target_link_libraries(scale_encode_append_test
    scale_encode_append
    )

addtest(scale_compact_test
    scale_compact_test.cpp
    )
target_link_libraries(scale_compact_test
    scale
    buffer
    blob
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

addtest(scale_strings_test
    scale_strings_test.cpp
    )
target_link_libraries(scale_strings_test
    scale
    )

addtest(scale_convenience_functions_test
    scale_convenience_functions_test.cpp
    )
target_link_libraries(scale_convenience_functions_test
    scale
    )

addtest(scale_tuple_test
    scale_tuple_test.cpp
    )
target_link_libraries(scale_tuple_test
    scale
    )

addtest(scale_array_test
    scale_array_test.cpp
    )
target_link_libraries(scale_array_test
    scale
    )

addtest(scale_encode_counter_test
    scale_encode_counter_test.cpp
    )
target_link_libraries(scale_encode_counter_test
    scale
    )
