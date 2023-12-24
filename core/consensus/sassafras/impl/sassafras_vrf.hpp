/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/bandersnatch/vrf.hpp"

namespace kagome::consensus::sassafras::vrf {

  using namespace crypto::bandersnatch::vrf;

  /// VRF input to generate the ticket id.
  VrfInput ticket_id_input(const Randomness &randomness,
                           AttemptsNumber attempt,
                           EpochNumber epoch);

  // clang-format off
////============================================================================
//
//  /// Ring VRF domain size for Sassafras consensus.
//static constexpr size_t RING_VRF_DOMAIN_SIZE = 2048;
//
///// Make ticket-id from the given VRF input and output.
/////
///// Input should have been obtained via [`ticket_id_input`].
///// Output should have been obtained from the input directly using the vrf secret key
///// or from the vrf signature outputs.
//TicketId make_ticket_id(const VrfInput& input, const VrfOutput& output) {
//  let bytes = output.make_bytes::<16>(b"ticket-id", input);
//  u128::from_le_bytes(bytes)
//}
//
//
///// ============================================================================
//
//
///// Bandersnatch VRF [`RingContext`] specialization for Sassafras using [`RING_VRF_DOMAIN_SIZE`].
//pub type RingContext = sp_core::bandersnatch::ring_vrf::RingContext<RING_VRF_DOMAIN_SIZE>;
//
//fn vrf_input_from_data(
//	domain: &[u8],
//	data: impl IntoIterator<Item = impl AsRef<[u8]>>,
//) -> VrfInput {
//	let buf = data.into_iter().fold(Vec::new(), |mut buf, item| {
//		let bytes = item.as_ref();
//		buf.extend_from_slice(bytes);
//		let len = u8::try_from(bytes.len()).expect("private function with well known inputs; qed");
//		buf.push(len);
//		buf
//	});
//	VrfInput::new(domain, buf)
//}
//
///// VRF input to claim slot ownership during block production.
//pub fn slot_claim_input(randomness: &Randomness, slot: Slot, epoch: u64) -> VrfInput {
//	vrf_input_from_data(
//		b"sassafras-claim-v1.0",
//		[randomness.as_slice(), &slot.to_le_bytes(), &epoch.to_le_bytes()],
//	)
//}
//
///// Signing-data to claim slot ownership during block production.
//pub fn slot_claim_sign_data(randomness: &Randomness, slot: Slot, epoch: u64) -> VrfSignData {
//	let input = slot_claim_input(randomness, slot, epoch);
//	VrfSignData::new_unchecked(
//		b"sassafras-slot-claim-transcript-v1.0",
//		Option::<&[u8]>::None,
//		Some(input),
//	)
//}
//
///// VRF input to generate the ticket id.
//pub fn ticket_id_input(randomness: &Randomness, attempt: u32, epoch: u64) -> VrfInput {
//	vrf_input_from_data(
//		b"sassafras-ticket-v1.0",
//		[randomness.as_slice(), &attempt.to_le_bytes(), &epoch.to_le_bytes()],
//	)
//}
//
///// VRF input to generate the revealed key.
//pub fn revealed_key_input(randomness: &Randomness, attempt: u32, epoch: u64) -> VrfInput {
//	vrf_input_from_data(
//		b"sassafras-revealed-v1.0",
//		[randomness.as_slice(), &attempt.to_le_bytes(), &epoch.to_le_bytes()],
//	)
//}
//
///// Data to be signed via ring-vrf.
//pub fn ticket_body_sign_data(ticket_body: &TicketBody, ticket_id_input: VrfInput) -> VrfSignData {
//	VrfSignData::new_unchecked(
//		b"sassafras-ticket-body-transcript-v1.0",
//		Some(ticket_body.encode().as_slice()),
//		Some(ticket_id_input),
//	)
//}
//
//
///// Make revealed key seed from a given VRF input and ouput.
/////
///// Input should have been obtained via [`revealed_key_input`].
///// Output should have been obtained from the input directly using the vrf secret key
///// or from the vrf signature outputs.
//pub fn make_revealed_key_seed(input: &VrfInput, output: &VrfOutput) -> [u8; 32] {
//	output.make_bytes::<32>(b"revealed-seed", input)
//}




}  // namespace kagome::consensus::sassafras::vrf
