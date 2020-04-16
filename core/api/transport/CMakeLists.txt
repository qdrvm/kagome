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

add_library(api_transport
    impl/http/http_session.hpp
    impl/http/http_session.cpp
    impl/ws/ws_session.hpp
    impl/ws/ws_session.cpp
    error.hpp
    error.cpp
    listener.hpp
    session.hpp
    impl/http/http_listener_impl.hpp
    impl/http/http_listener_impl.cpp
    impl/ws/ws_listener_impl.hpp
    impl/ws/ws_listener_impl.cpp
    )
target_link_libraries(api_transport
    Boost::boost
    logger
    )
