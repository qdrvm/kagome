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

add_subdirectory(vote_graph)

add_library(environment
    impl/environment_impl.cpp
    )

target_link_libraries(environment
    scale
    logger
    blob
    )

add_library(vote_tracker
    impl/vote_tracker_impl.cpp
    )
target_link_libraries(vote_tracker
    blob
    )

add_library(vote_crypto_provider
    impl/vote_crypto_provider_impl.cpp
    )
target_link_libraries(vote_crypto_provider
    scale
    )

add_library(voter_set
    voter_set.cpp
    )
target_link_libraries(voter_set
    ed25519_types
    )

add_library(vote_weight
    vote_weight.cpp
    )
target_link_libraries(vote_weight
    voter_set
    )

add_library(voting_round_error
    impl/voting_round_error.cpp
    )
target_link_libraries(voting_round_error
    outcome
    )

add_library(voting_round
    impl/voting_round_impl.cpp
    )

target_link_libraries(voting_round
    logger
    voter_set
    voting_round_error
    )

add_library(launcher
    impl/launcher_impl.cpp
    )
target_link_libraries(launcher
    voting_round
    vote_crypto_provider
    vote_graph
    vote_tracker
    )
