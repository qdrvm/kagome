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

addtest(block_request_test
    blocks_request_test.cpp
    )
target_link_libraries(block_request_test
    buffer
    scale
    testutil_primitives_generator
    )

addtest(block_response_test
    blocks_response_test.cpp
    )
target_link_libraries(block_response_test
    buffer
    scale
    testutil_primitives_generator
    )

addtest(block_announce_test
    block_announce_test.cpp
    )
target_link_libraries(block_announce_test
    buffer
    scale
    testutil_primitives_generator
    )

addtest(block_attributes_test
    block_attributes_test.cpp
    )
target_link_libraries(block_attributes_test
    scale
    )

addtest(block_direction_test
    block_direction_test.cpp
    )
target_link_libraries(block_direction_test
    scale
    )
