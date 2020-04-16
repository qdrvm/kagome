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

add_library(extrinsic_api_service
    error.cpp
    error.hpp
    extrinsic_api.hpp
    extrinsic_jrpc_processor.hpp
    extrinsic_jrpc_processor.cpp
    impl/extrinsic_api_impl.hpp
    impl/extrinsic_api_impl.cpp
    request/submit_extrinsic.hpp
    request/submit_extrinsic.cpp
    )
target_link_libraries(extrinsic_api_service
    buffer
    api_service
    transaction_pool
    scale
    logger
    )
