#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(byte_array_stream
    byte_array_stream.hpp
    byte_array_stream.cpp
    )
target_link_libraries(byte_array_stream
    byte_stream
    )

add_library(scale
    blob_codec.hpp
    boolean.hpp
    boolean.cpp
    byte_array_stream.hpp
    byte_array_stream.cpp
    collection.hpp
    collection.cpp
    compact.hpp
    compact.cpp
    fixedwidth.hpp
    fixedwidth.cpp
    optional.hpp
    optional_bool.hpp
    optional_bool.cpp
    scale.hpp
    scale_error.hpp
    scale_error.cpp
    types.hpp
    type_decoder.hpp
    type_encoder.hpp
    util.hpp
    variant.hpp
    scale_encoder_stream.cpp scale_encoder_stream.hpp scale_decoder_stream.cpp scale_decoder_stream.hpp scale_stream.hpp)

target_link_libraries(scale
    Boost::boost
    buffer
    outcome
    byte_array_stream
    )

add_library(scale_buffer_codec
    buffer_codec.cpp
    )
target_link_libraries(scale_buffer_codec
    scale
    )
