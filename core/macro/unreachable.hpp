/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_UNREACHABLE_HPP
#define KAGOME_UNREACHABLE_HPP

/**
 * @brief This file declares UNREACHABLE macro. Use it to prevent compiler
 * warnings.
 */

#if defined(__GNUC__)
#define UNREACHABLE __builtin_unreachable();
#elif defined(_MSC_VER)
#define UNREACHABLE __assume(false);
#else
template <unsigned int LINE>
class Unreachable_At_Line {};
#define UNREACHABLE throw Unreachable_At_Line<__LINE__>();  // NOLINT
#endif

#undef GCC_VERSION

#endif  // KAGOME_UNREACHABLE_HPP
