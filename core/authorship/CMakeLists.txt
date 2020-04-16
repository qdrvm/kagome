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

add_library(block_builder
    block_builder.hpp
    impl/block_builder_impl.cpp
    impl/block_builder_impl.hpp
    impl/block_builder_error.cpp
    impl/block_builder_error.hpp
    )
target_link_libraries(block_builder
    outcome
    buffer
    logger
    )

add_library(block_builder_factory
    block_builder_factory.hpp
    impl/block_builder_factory_impl.cpp
    impl/block_builder_factory_impl.hpp
    )
target_link_libraries(block_builder_factory
    block_builder
    block_header_repository
    )

add_library(proposer
    proposer.hpp
    impl/proposer_impl.cpp
    impl/proposer_impl.hpp
    )
target_link_libraries(proposer
    block_builder_factory
    )
