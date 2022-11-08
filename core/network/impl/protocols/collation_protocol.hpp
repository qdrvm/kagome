/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_COLLATIONPROTOCOL
#define KAGOME_NETWORK_COLLATIONPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/collation_observer.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/parachain_protocol.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/roles.hpp"
#include "network/types/status.hpp"
#include "network/validation_observer.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  using CollationProtocol =
      ParachainProtocol<CollationObserver, CollationProtocolMessage>;

  using ValidationProtocol =
      ParachainProtocol<ValidationObserver, ValidatorProtocolMessage>;

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_COLLATIONPROTOCOL
