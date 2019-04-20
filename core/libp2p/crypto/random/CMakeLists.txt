#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(random_provider
        random_generator.hpp
        cryptographic_safe_generator.hpp
        impl/boost_generator.hpp
        impl/boost_generator.cpp
        impl/std_generator.hpp
        impl/std_generator.cpp
        )

target_link_libraries(random_provider
        Boost::random
        )
