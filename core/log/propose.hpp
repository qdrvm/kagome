#pragma once

#include <boost/algorithm/string/predicate.hpp>

#include "authorship/impl/proposer_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "network/impl/synchronizer_impl.hpp"
#include "parachain/parachain_inherent_data.hpp"
#include "primitives/block_header.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/inherent_data.hpp"

#define GLOBAL(type, name, ...)    \
  inline auto &name() {            \
    using Type = type;             \
    static Type name{__VA_ARGS__}; \
    return name;                   \
  }
#define GLOBALF(type, name, f) GLOBAL(type, name, []() -> Type f())

namespace kagome::propose {
  using authorship::ProposerImpl;
  using blockchain::BlockTreeImpl;
  using common::hex_lower;
  using network::BlockAttribute;
  using network::BlocksRequest;
  using network::BlocksResponse;
  using network::Direction;
  using network::SynchronizerImpl;
  using parachain::ParachainInherentData;
  using primitives::BlockInfo;
  using primitives::Digest;
  using primitives::Extrinsic;
  using primitives::InherentData;
  using primitives::InherentIdentifier;

  struct Tuple {
    SCALE_TIE(4);
    BlockInfo parent;
    InherentData inherent;
    Digest pre;
    std::vector<Extrinsic> ext;
  };

  GLOBALF(std::optional<Tuple>, tuple, {
    if (auto s = getenv("PROPOSE")) {
      return scale::decode<Tuple>(common::unhex(s).value()).value();
    }
    return {};
  });

  GLOBAL(SynchronizerImpl *, synchronizer);
  GLOBAL(ProposerImpl *, proposer);

  GLOBAL(std::mutex, mutex);

  inline void propose() {
    if (not tuple()) {
      return;
    }
    if (not synchronizer()) {
      return;
    }
    if (not proposer()) {
      return;
    }
    std::unique_lock lock{mutex()};
    if (synchronizer()->state_sync_) {
      return;
    }
    auto &tree = dynamic_cast<BlockTreeImpl &>(*synchronizer()->block_tree_);
    auto _h = tree.getBlockHeader(tuple()->parent.hash);
    auto num = tuple()->parent.number;
    if (not _h) {
      BlocksRequest req{
          BlockAttribute::HEADER,
          num,
          Direction::DESCENDING,
          1,
      };
      if (auto p =
              synchronizer()->chooseJustificationPeer(num, req.fingerprint())) {
        fmt::println("PROPOSE: header {}: try peer {}", num, *p);
        lock.unlock();
        synchronizer()->fetch(
            *p,
            req,
            "propose",
            [&tree, num](outcome::result<BlocksResponse> _r) {
              if (not _r) {
                fmt::println("PROPOSE: header {}: error {}", num, _r.error());
                propose();
                return;
              }
              auto &r = _r.value().blocks;
              if (r.empty()) {
                fmt::println("PROPOSE: header {}: no blocks", num);
                propose();
                return;
              }
              if (not r[0].header) {
                fmt::println("PROPOSE: header {}: no header", num);
                propose();
                return;
              }
              auto &h = *r[0].header;
              calculateBlockHash(h, *synchronizer()->hasher_);
              if (h.hash() != tuple()->parent.hash) {
                fmt::println("PROPOSE: header {}: wrong hash", num);
                propose();
                return;
              }
              tree.block_tree_data_.block_tree_data_.unsafeGet()
                  .storage_->putBlockHeader(h)
                  .value();
              propose();
            });
      }
      fmt::println("PROPOSE: header {}: no peer", num);
      return;
    }
    fmt::println("PROPOSE: header {}: ok", num);
    auto &h = _h.value();
    if (not synchronizer()->storage_->getEphemeralBatchAt(h.state_root)) {
      if (auto p = synchronizer()->chooseJustificationPeer(num, {})) {
        fmt::println("PROPOSE: state {}: try peer {}", num, *p);
        lock.unlock();
        synchronizer()->syncState(
            *p, tuple()->parent, [num](outcome::result<BlockInfo> _r) {
              if (not _r) {
                fmt::println("PROPOSE: state {}: error {}", num, _r.error());
                propose();
                return;
              }
              propose();
            });
      }
      fmt::println("PROPOSE: state {}: no peer", num);
      return;
    }
    fmt::println("PROPOSE: state {}: ok", num);

    std::array<uint8_t, 8> parachn0{'p', 'a', 'r', 'a', 'c', 'h', 'n', '0'};
    auto para =
        tuple()
            ->inherent
            .getData<ParachainInherentData>(InherentIdentifier{parachn0})
            .value();
    fmt::println("PROPOSE: candidate={} bitfield={} dispute={}",
                 para.backed_candidates.size(),
                 para.bitfields.size(),
                 para.disputes.size());

    auto bb =
        proposer()
            ->block_builder_factory_->make(tuple()->parent, tuple()->pre, {})
            .value();
    auto inherent = bb->getInherentExtrinsics(tuple()->inherent).value();
    if (not boost::starts_with(tuple()->ext, inherent)) {
      fmt::println("PROPOSE: wrong inherent extrinsics");
      abort();
    }
    auto i = 0;
    for (auto &x : tuple()->ext) {
      if (auto r = bb->pushExtrinsic(x)) {
        fmt::println("PROPOSE: ext {} [{}]: ok", i, hex_lower(x.data));
      } else {
        fmt::println(
            "PROPOSE: ext {} [{}]: error {}", i, hex_lower(x.data), r.error());
      }
      ++i;
    }
    if (auto r = bb->bake()) {
      fmt::println("PROPOSE: finalize: ok");
    } else {
      fmt::println("PROPOSE: finalize: error {}", r.error());
    }
    kill(getpid(), SIGKILL);
  }
}  // namespace kagome::propose

#undef GLOBAL
#undef GLOBAL_V
