#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(telemetry
    impl/service_impl.cpp
    impl/connection_impl.cpp
    )
target_link_libraries(telemetry
    Boost::boost
    Boost::date_time
    OpenSSL::SSL
    )
