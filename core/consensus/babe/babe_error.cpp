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

#include "consensus/babe/babe_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BabeError, e) {
  using E = kagome::consensus::BabeError;
  switch (e) {
    case E::TIMER_ERROR:
      return "some internal error happened while using the timer in BABE; "
             "please, see logs";
    case E::NODE_FALL_BEHIND:
      return "local node has fallen too far behind the others, most likely "
             "it is in one of the previous epochs";
  }
  return "unknown error";
}
