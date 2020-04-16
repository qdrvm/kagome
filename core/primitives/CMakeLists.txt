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

add_library(primitives
    extrinsic_api_primitives.hpp
    block.hpp
    block_header.hpp
    block_id.hpp
    common.hpp
    check_inherents_result.hpp
    digest.hpp
    extrinsic.hpp
    inherent_data.cpp
    inherent_data.hpp
    parachain_host.hpp
    scheduled_change.hpp
    transaction_validity.hpp
    transaction_validity.cpp
    version.hpp
    )
target_link_libraries(primitives
    buffer
    scale
    blob
    )
