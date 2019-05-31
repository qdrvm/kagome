#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(scale
    optional_bool.hpp
    optional_bool.cpp
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
    outcome
    )
