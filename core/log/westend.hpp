#pragma once

#include <libp2p/peer/peer_id.hpp>

#include "crypto/hasher/hasher_impl.hpp"
#include "log/logger.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "primitives/authority_discovery_id.hpp"

#define IF_LOG_WESTEND \
  if (not ::kagome::log::westend::log()) return

namespace kagome::log::westend {
  using libp2p::PeerId;
  using network::candidateHash;
  using network::View;
  using Audi = primitives::AuthorityDiscoveryId;

  inline const crypto::HasherImpl hasher;

  inline auto &log() {
    static Logger log;
    return log;
  }

  inline void chainSpec(std::string_view chain_spec_id) {
    if (not chain_spec_id.starts_with("westend")) {
      return;
    }
    log() = createLogger("WESTEND");
    log()->info("verbose debug logs for westend");
  }

  inline auto _peer(const PeerId &peer) {
    return peer.toBase58();
  }

  inline void audi(const Audi &audi, const PeerId &peer) {
    IF_LOG_WESTEND;
    log()->info("audi(audi={}, peer={})", audi.toHex(), _peer(peer));
  }

  inline void connected(const PeerId &peer, bool out) {
    IF_LOG_WESTEND;
    log()->info("connected(peer={}, out={})", _peer(peer), out);
  }

  inline void disconnected(const PeerId &peer) {
    IF_LOG_WESTEND;
    log()->info("disconnected(peer={})", _peer(peer));
  }

  inline void stream_open(const PeerId &peer, bool out, bool collation) {
    IF_LOG_WESTEND;
    log()->info("stream_open(peer={}, out={}, collation={})",
                _peer(peer),
                out,
                collation);
  }

  inline void stream_close(const PeerId &peer, bool out, bool collation) {
    IF_LOG_WESTEND;
    log()->info("stream_close(peer={}, out={}, collation={})",
                _peer(peer),
                out,
                collation);
  }

  inline auto _view(const View &view) {
    std::vector<std::string> heads;
    for (auto &head : view.heads_) {
      heads.emplace_back(head.toHex());
    }
    return fmt::format("view(heads=[{}], finalized={})",
                       fmt::join(heads, ", "),
                       view.finalized_number_);
  }

  inline auto write_view(const View &view) {
    struct F {
      std::string view;
      std::vector<std::string> peers;
      auto &operator()(const PeerId &peer) {
        IF_LOG_WESTEND *this;
        peers.emplace_back(_peer(peer));
        return *this;
      }
      void operator()() {
        IF_LOG_WESTEND;
        log()->info(
            "write_view(view={}, peers=[{}])", view, fmt::join(peers, ", "));
      }
    };
    IF_LOG_WESTEND F{};
    return F{_view(view), {}};
  }

  inline void read_view(const PeerId &peer, const View &view) {
    IF_LOG_WESTEND;
    log()->info("read_view(peer={}, view={})", _peer(peer), _view(view));
  }

#define _WEST_MSG(T) inline auto _message(const T &m)
#define _WEST_CASE(T) [](const T &m) -> std::string
#define _WEST_PASS(T) \
  _WEST_CASE(T) { return _message(m); }
  _WEST_MSG(network::Statement) {
    return visit_in_place(
        m.candidate_state,
        _WEST_CASE(network::CommittedCandidateReceipt) {
          return fmt::format("Seconded(candidate={})",
                             candidateHash(hasher, m).toHex());
        },
        _WEST_CASE(network::CandidateHash) {
          return fmt::format("Valid(candidate={})", m.toHex());
        },
        _WEST_CASE(auto) { abort(); });
  }
  _WEST_MSG(network::CollatorDeclaration) {
    return fmt::format("Declare(para={})", m.para_id);
  }
  _WEST_MSG(network::Seconded) {
    return fmt::format("Seconded(relay={}, statement={}, i={})",
                       m.relay_parent.toHex(),
                       _message(m.statement.payload.payload),
                       m.statement.payload.ix);
  }
  _WEST_MSG(network::CollationMessage0) {
    return visit_in_place(
        boost::get<network::CollationMessage>(m),
        _WEST_PASS(network::CollatorDeclaration),
        _WEST_CASE(network::CollatorAdvertisement) {
          return fmt::format("Advertise(relay={})", m.relay_parent.toHex());
        },
        _WEST_PASS(network::Seconded),
        _WEST_CASE(auto) { abort(); });
  }
  _WEST_MSG(network::vstaging::CollationMessage0) {
    return visit_in_place(
        boost::get<network::vstaging::CollationMessage>(m),
        _WEST_PASS(network::CollatorDeclaration),
        _WEST_CASE(
            network::vstaging::CollatorProtocolMessageAdvertiseCollation) {
          return fmt::format("Advertise(relay={}, candidate={})",
                             m.relay_parent.toHex(),
                             m.candidate_hash.toHex());
        },
        _WEST_PASS(network::Seconded),
        _WEST_CASE(auto) { abort(); });
  }
  _WEST_MSG(network::ApprovalDistributionMessage) {
    return visit_in_place(
        m,
        _WEST_CASE(network::Assignments) {
          return fmt::format("Assignments(len={})", m.assignments.size());
        },
        _WEST_CASE(network::Approvals) {
          return fmt::format("Approvals(len={})", m.approvals.size());
        });
  }
  _WEST_MSG(network::vstaging::ApprovalDistributionMessage) {
    return visit_in_place(
        m,
        _WEST_CASE(network::vstaging::Assignments) {
          return fmt::format("Assignments(len={})", m.assignments.size());
        },
        _WEST_CASE(network::vstaging::Approvals) {
          return fmt::format("Approvals(len={})", m.approvals.size());
        });
  }
  _WEST_MSG(network::BitfieldDistribution) {
    return fmt::format(
        "Bitfield(relay={}, i={})", m.relay_parent.toHex(), m.data.payload.ix);
  }
  _WEST_MSG(network::BitfieldDistributionMessage) {
    return _message(boost::get<network::BitfieldDistribution>(m));
  }
  _WEST_MSG(network::vstaging::StatementDistributionMessageStatement) {
    return fmt::format(
        "StatementDistributionMessageStatement(relay={}, statement={}, i={})",
        m.relay_parent.toHex(),
        visit_in_place(
            m.compact.payload.payload.inner_value,
            _WEST_CASE(network::vstaging::SecondedCandidateHash) {
              return fmt::format("Seconded(candidate={})", m.hash.toHex());
            },
            _WEST_CASE(network::vstaging::ValidCandidateHash) {
              return fmt::format("Valid(candidate={})", m.hash.toHex());
            },
            _WEST_CASE(auto) { abort(); }),
        m.compact.payload.ix);
  }
  _WEST_MSG(network::vstaging::BackedCandidateManifest) {
    return fmt::format(
        "BackedCandidateManifest(relay={}, candidate={}, group={}, para={})",
        m.relay_parent.toHex(),
        m.candidate_hash.toHex(),
        m.group_index,
        m.para_id);
  }
  _WEST_MSG(network::vstaging::BackedCandidateAcknowledgement) {
    return fmt::format("BackedCandidateAcknowledgement(candidate={})",
                       m.candidate_hash.toHex());
  }
  _WEST_MSG(network::LargeStatement) {
    return fmt::format("LargeStatement(relay={}, candidate={}, i={})",
                       m.payload.payload.relay_parent.toHex(),
                       m.payload.payload.candidate_hash.toHex(),
                       m.payload.ix);
  }
  _WEST_MSG(network::StatementDistributionMessage) {
    return fmt::format("Statement({})",
                       visit_in_place(m,
                                      _WEST_PASS(network::Seconded),
                                      _WEST_PASS(network::LargeStatement)));
  }
  _WEST_MSG(network::vstaging::StatementDistributionMessage) {
    return fmt::format(
        "Statement({})",
        visit_in_place(
            m,
            _WEST_PASS(
                network::vstaging::StatementDistributionMessageStatement),
            _WEST_PASS(network::vstaging::BackedCandidateManifest),
            _WEST_PASS(network::vstaging::BackedCandidateAcknowledgement)));
  }
  _WEST_MSG(network::ValidatorProtocolMessage) {
    return visit_in_place(
        m,
        _WEST_PASS(network::BitfieldDistributionMessage),
        _WEST_PASS(network::StatementDistributionMessage),
        _WEST_PASS(network::ApprovalDistributionMessage),
        _WEST_CASE(auto) { abort(); });
  }
  _WEST_MSG(network::vstaging::ValidatorProtocolMessage) {
    return visit_in_place(
        m,
        _WEST_PASS(network::BitfieldDistributionMessage),
        _WEST_PASS(network::vstaging::StatementDistributionMessage),
        _WEST_PASS(network::vstaging::ApprovalDistributionMessage),
        _WEST_CASE(auto) { abort(); });
  }
  _WEST_MSG(network::VersionedValidatorProtocolMessage) {
    return visit_in_place(m, _WEST_PASS(auto));
  }

  void read_message(const PeerId &peer, const auto &m) {
    IF_LOG_WESTEND;
    log()->info("read_message(peer={}, message={})", _peer(peer), _message(m));
  }

  auto write_message(const auto &message) {
    struct F {
      std::string message;
      std::vector<std::string> peers;
      auto &operator()(const PeerId &peer) {
        IF_LOG_WESTEND *this;
        peers.emplace_back(_peer(peer));
        return *this;
      }
      void operator()() {
        IF_LOG_WESTEND;
        log()->info("write_message(message={}, peers=[{}])",
                    message,
                    fmt::join(peers, ", "));
      }
    };
    IF_LOG_WESTEND F{};
    return F{_message(message), {}};
  }
}  // namespace kagome::log::westend
