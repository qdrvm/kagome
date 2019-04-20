#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(random_provider
        pseudo_random_generator.hpp
        cryptographically_secure_generator.hpp
        impl/boost_generator.hpp
        impl/boost_generator.cpp
        impl/std_generator.hpp
        impl/std_generator.cpp
        )

target_link_libraries(random_provider
        Boost::random
        )
