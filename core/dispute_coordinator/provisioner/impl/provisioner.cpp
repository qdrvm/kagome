/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_PRIORITIZEDSELECTION
#define KAGOME_DISPUTE_PRIORITIZEDSELECTION

#include "dispute_coordinator/provisioner/impl/provisioner.hpp"

#include "dispute_coordinator/types.hpp"
#include "scale/bitvec.hpp"

namespace kagome::dispute {

  void Provisioner::handle_RequestInherentData(
      const primitives::BlockHash &relay_parent) {
    // SL_TRACE(
    //     log_,
    //     "Inherent data got requested for relay parent {}",
    //     relay_parent);

    auto it = per_relay_parent_.find(relay_parent);
    if (it == per_relay_parent_.end()) {
      return;
    }

    auto &state = it->second;

    if (state.is_inherent_ready) {
      // SL_TRACE(
      //     log_,
      //     "Calling send_inherent_data for relay parent {}",
      //     relay_parent);

      auto res = send_inherent_data_bg(state);
      if (res.has_error()) {
        // SL_TRACE(
        //     log_,
        //     "send_inherent_data return error: {}",
        //     res.error());
      }

    } else {
      // SL_TRACE(
      //     log_,
      //     "Queuing inherent data request for relay parent {} "
      //     "(inherent data not yet ready)",
      //     relay_parent);

      // state.awaiting_inherent.push(return_sender); // FIXME
    }
  }

  void Provisioner::handle_ProvisionableData(
      const primitives::BlockHash &relay_parent, ProvisionableData data) {
    auto it = per_relay_parent_.find(relay_parent);
    if (it == per_relay_parent_.end()) {
      return;
    }

    auto &state = it->second;

    // TODO need to be implemented

    /* clang-format off

      let span = state.span.child("provisionable-data");
      let _timer = metrics.time_provisionable_data();

      gum::trace!(target: LOG_TARGET, ?relay_parent, "Received provisionable data.");

      note_provisionable_data(state, &span, data);

    clang-format on */
  }

  void Provisioner::on_active_leaves_update(const network::ExView &updated) {
    for (auto &deactivated : updated.lost) {
      per_relay_parent_.erase(deactivated);
    }

    if (updated.new_head_hash.has_value()) {
      ActivatedLeaf leaf{
          .hash = updated.new_head_hash.value(),
          .number = updated.new_head.number,
          .status = LeafStatus::Fresh,
      };
      per_relay_parent_.emplace(leaf.hash, PerRelayParent{.leaf = leaf});

      // let delay_fut =
      //      Delay::new(PRE_PROPOSE_TIMEOUT).map(move |_| leaf.hash).boxed();
      // inherent_delays.push(delay_fut);
    }
  }

  outcome::result<void> Provisioner::send_inherent_data_bg(
      kagome::dispute::PerRelayParent &per_relay_parent) {
    // TODO need to be implemented

    /* clang-format off

    let leaf = per_relay_parent.leaf.clone();
    let signed_bitfields = per_relay_parent.signed_bitfields.clone();
    let backed_candidates = per_relay_parent.backed_candidates.clone();
    let span = per_relay_parent.span.child("req-inherent-data");

    let mut sender = ctx.sender().clone();

    let bg = async move {
      let _span = span;
      let _timer = metrics.time_request_inherent_data();

      gum::trace!(
        target: LOG_TARGET,
        relay_parent = ?leaf.hash,
        "Sending inherent data in background."
      );

      let send_result = send_inherent_data(
        &leaf,
        &signed_bitfields,
        &backed_candidates,
        return_senders,
        &mut sender,
        &metrics,
      ) // Make sure call is not taking forever:
      .timeout(SEND_INHERENT_DATA_TIMEOUT)
      .map(|v| match v {
        Some(r) => r,
        None => Err(Error::SendInherentDataTimeout),
      });

      match send_result.await {
        Err(err) => {
          if let Error::CanceledBackedCandidates(_) = err {
            gum::debug!(
              target: LOG_TARGET,
              err = ?err,
              "Failed to assemble or send inherent data - block got likely obsoleted already."
            );
          } else {
            gum::warn!(target: LOG_TARGET, err = ?err, "failed to assemble or send inherent data");
          }
          metrics.on_inherent_data_request(Err(()));
        },
        Ok(()) => {
          metrics.on_inherent_data_request(Ok(()));
          gum::debug!(
            target: LOG_TARGET,
            signed_bitfield_count = signed_bitfields.len(),
            backed_candidates_count = backed_candidates.len(),
            leaf_hash = ?leaf.hash,
            "inherent data sent successfully"
          );
          metrics.observe_inherent_data_bitfields_count(signed_bitfields.len());
        },
      }
    };

    ctx.spawn("send-inherent-data", bg.boxed())
      .map_err(|_| Error::FailedToSpawnBackgroundTask)?;

    Ok(())

    clang-format on */
  }

}  // namespace kagome::dispute
