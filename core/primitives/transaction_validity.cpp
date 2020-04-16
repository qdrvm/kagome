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

#include "primitives/transaction_validity.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, InvalidTransaction, e) {
  using E = kagome::primitives::InvalidTransaction;
  switch (e) {
    case E::Call:
      return "The call of the transaction is not expected";
    case E::Payment:
      return "General error to do with the inability to pay some fees (e.g. "
             "account balance too low)";
    case E::Future:
      return "General error to do with the transaction not yet being valid "
             "(e.g. nonce too high)";
    case E::Stale:
      return "General error to do with the transaction being outdated (e.g. "
             "nonce too low)";
    case E::BadProof:
      return "General error to do with the transaction's proofs (e.g. "
             "signature)";
    case E::AncientBirthBlock:
      return "The transaction birth block is ancient";
    case E::ExhaustsResources:
      return "The transaction would exhaust the resources of current block. "
             "The transaction might be valid, but there are not enough "
             "resources left in the current block";
    case E::Custom:
      return "Custom invalid error";
  }
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, UnknownTransaction, e) {
  using E = kagome::primitives::UnknownTransaction;
  switch (e) {
    case E::CannotLookup:
      return "Could not lookup some information that is required to validate "
             "the transaction";
    case E::NoUnsignedValidator:
      return "No validator found for the given unsigned transaction";
    case E::Custom:
      return "Custom unknown error";
  }
}
