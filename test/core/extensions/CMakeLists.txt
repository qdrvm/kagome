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

addtest(crypto_extension_test
    crypto_extension_test.cpp
    )
target_link_libraries(crypto_extension_test
    crypto_extension
    )

addtest(io_extension_test
    io_extension_test.cpp
    )
target_link_libraries(io_extension_test
    io_extension
    )

addtest(memory_extension_test memory_extension_test)
target_link_libraries(memory_extension_test
    memory_extension
    )

addtest(misc_extension_test
    misc_extension_test.cpp
    )
target_link_libraries(misc_extension_test
    misc_extension
    )

addtest(storage_extension_test
    storage_extension_test.cpp
    )
target_link_libraries(storage_extension_test
    storage_extension
    )
