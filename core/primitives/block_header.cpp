#include "primitives/block_header.hpp"

namespace kagome::primitives {

  outcome::result<BlockHash> calculateBlockHash(BlockHeader const &header,
                                                crypto::Hasher const &hasher) {
    OUTCOME_TRY(enc_header, scale::encode(header));
    return hasher.blake2b_256(enc_header);
  }

}  // namespace kagome::primitives
