#pragma once

#include <boost/functional/hash.hpp>
#include "parachain/validator/blocked_collation_id.hpp"

// Hash specialization for BlockedCollationId
namespace std {
  template <>
  struct hash<kagome::parachain::BlockedCollationId> {
    size_t operator()(
        const kagome::parachain::BlockedCollationId &value) const {
      using Hash = kagome::parachain::Hash;
      using ParachainId = kagome::parachain::ParachainId;

      size_t result = std::hash<ParachainId>()(value.para_id);
      boost::hash_combine(result,
                          std::hash<Hash>()(value.parent_head_data_hash));
      return result;
    }
  };
}  // namespace std
