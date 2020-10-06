#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(scale
    scale.hpp
    scale_decoder_stream.hpp
    scale_decoder_stream.cpp
    scale_encoder_stream.hpp
    scale_encoder_stream.cpp
    scale_error.hpp
    scale_error.cpp
    types.hpp
    detail/fixed_witdh_integer.hpp
    detail/variant.hpp
    )

target_link_libraries(scale
    Boost::boost
    buffer
    )
kagome_install(scale)

add_library(scale_encode_append
    encode_append.cpp
    )
target_link_libraries(scale_encode_append
    scale
    )
kagome_install(scale_encode_append)
