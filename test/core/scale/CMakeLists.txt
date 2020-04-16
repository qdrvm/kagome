##
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
##

addtest(scale_decoder_stream_test
    scale_decoder_stream_test.cpp
    )
target_link_libraries(scale_decoder_stream_test
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
