#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(clock
    impl/clock_impl.cpp
    )
kagome_install(clock)

add_library(waitable_timer
    impl/basic_waitable_timer.hpp
    impl/basic_waitable_timer.cpp
    )
target_link_libraries(waitable_timer
    Boost::boost
    )

add_library(ticker
    impl/ticker_impl.cpp
    impl/ticker_impl.hpp
    )
target_link_libraries(ticker
    Boost::boost
    )
