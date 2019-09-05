#
# Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/boostorg/beast
#

add_executable(multiserver
    beast.hpp
    http_session.cpp
    http_session.hpp
    beast_listener.cpp
    beast_listener.hpp
    main.cpp
    net.hpp
    websocket_session.cpp
    websocket_session.hpp
    )
target_link_libraries(multiserver
    Boost::boost
    )
