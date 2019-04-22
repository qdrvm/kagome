#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(random_provider
    prng.hpp
    csprng.hpp
    impl/boost_generator.hpp
    impl/boost_generator.cpp
    impl/std_generator.hpp
    impl/std_generator.cpp
    random_generator.hpp
    )

target_link_libraries(random_provider
    buffer
    Boost::random
    )
