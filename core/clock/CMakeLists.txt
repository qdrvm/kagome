#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(system_clock
    system_clock.hpp
    impl/system_clock_impl.cpp
    impl/system_clock_impl.hpp
    )

add_library(steady_clock
    steady_clock.hpp
    impl/steady_clock_impl.cpp
    impl/steady_clock_impl.hpp
    )
