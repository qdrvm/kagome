#include "primitives/block_header.hpp"

namespace kagome::primitives {

  void calculateBlockHash(BlockHeader &header, const crypto::Hasher &hasher) {
    auto enc_res = scale::encode(header);
    BOOST_ASSERT_MSG(enc_res.has_value(), "Header should be encoded errorless");
    header.hash_opt.emplace(hasher.blake2b_256(enc_res.value()));
  }

}  // namespace kagome::primitives
