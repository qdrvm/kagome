#include "crypto/crypto_store/session_keys.hpp"

#include "crypto/crypto_store.hpp"

namespace kagome::crypto {

  SessionKeys::SessionKeys(std::shared_ptr<CryptoStore> store,
                           const network::Roles &roles)
      : store_(store), roles_(roles) {}

  SessionKeys::~SessionKeys() {
    std::cout << "Die, motherfucker, die!" << std::endl;
  }

  const std::shared_ptr<Sr25519Keypair>& SessionKeys::getBabeKeyPair() {
    if (!babe_key_pair_ && roles_.flags.authority) {
      auto keys = store_->getSr25519PublicKeys(KEY_TYPE_BABE);
      if (keys and not keys.value().empty()) {
        auto kp = store_->findSr25519Keypair(KEY_TYPE_BABE, keys.value().at(0));
        babe_key_pair_ = std::make_shared<Sr25519Keypair>(kp.value());
      }
    }
    return babe_key_pair_;
  }

}  // namespace kagome::crypto
