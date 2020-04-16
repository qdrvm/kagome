#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
#
# Kagome is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Kagome is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
