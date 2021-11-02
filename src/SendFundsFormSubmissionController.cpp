//
//  SendFundsFormSubmissionController.cpp
//  MyMonero
//
//  Copyright (c) 2014-2019, MyMonero.com
//
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification, are
//  permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//	of conditions and the following disclaimer in the documentation and/or other
//	materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its contributors may be
//	used to endorse or promote products derived from this software without specific
//	prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
//  THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//
#include "SendFundsFormSubmissionController.hpp"
#include <iostream>
#include "wallet_errors.h"
#include "monero_address_utils.hpp"
#include "monero_paymentID_utils.hpp"
#include "monero_send_routine.hpp"
#include "serial_bridge_utils.hpp"
using namespace monero_send_routine;
using namespace monero_transfer_utils;
using namespace SendFunds;
using namespace serial_bridge_utils;

#include <boost/optional/optional_io.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost;

string FormSubmissionController::handle(const property_tree::ptree res)
{
	this->randomOuts = res;
	// const bool prep = this->prepare();
	// if (!prep) {
	// 	return error_ret_json_from_message(this->failureReason);
	// }
	//this->_proceedTo_generateSendTransaction()
	const bool step2 = this->cb_II__got_random_outs(this->randomOuts);
	if (!step2) {
		return error_ret_json_from_message("couldnt use random outputs");
	}
	
	return this->cb_III__submitted_tx();
}

string FormSubmissionController::prepare()
{
	using namespace std;
	using namespace boost;
	//
	this->valsState = WAIT_FOR_STEP1;
	//
	if (this->parameters.is_sweeping) {
		this->sending_amount = 0;
	} else {
		if (this->parameters.send_amount_double_string == boost::none || this->parameters.send_amount_double_string->empty()) {
			if (this->parameters.is_sweeping == false) {
				return error_ret_json_from_message("Amount too low.");
			}
		}
		bool ok = cryptonote::parse_amount(this->sending_amount, this->parameters.send_amount_double_string.get());
		if (!ok) {
			return error_ret_json_from_message("Cannot parse amount.");
		}
		if (this->sending_amount <= 0) {
			return error_ret_json_from_message("Amount too low.");
		}
	}
	bool manuallyEnteredPaymentID_exists = this->parameters.manuallyEnteredPaymentID != boost::none && !this->parameters.manuallyEnteredPaymentID->empty();
	optional<string> paymentID_toUseOrToNilIfIntegrated = boost::none; // may be nil
	string xmrAddress_toDecode = this->parameters.enteredAddressValue;
	// we don't care whether it's an integrated address or not here since we're not going to use its payment id
	paymentID_toUseOrToNilIfIntegrated = this->parameters.manuallyEnteredPaymentID.get();

	auto decode_retVals = monero::address_utils::decodedAddress(xmrAddress_toDecode, this->parameters.nettype);
	if (decode_retVals.did_error) {
		return error_ret_json_from_message("Invalid address");
	}
	if (decode_retVals.paymentID_string != boost::none) { // is integrated address!
		this->to_address_string = xmrAddress_toDecode; // for integrated addrs, we don't want to extract the payment id and then use the integrated addr as well (TODO: unless we use fluffy's patch?)
		this->payment_id_string = boost::none;
		this->isXMRAddressIntegrated = true;
		this->integratedAddressPIDForDisplay = *decode_retVals.paymentID_string;
		//this->_proceedTo_generateSendTransaction();
		boost::property_tree::ptree root;
		root.put("retVal", "true");
	
		return ret_json_from_root(root);
	}
	// since we may have a payment ID here (which may also have been entered manually), validate
	// if (monero_paymentID_utils::is_a_valid_or_not_a_payment_id(paymentID_toUseOrToNilIfIntegrated) == false) { // convenience function - will be true if nil pid
	// 	return error_ret_json_from_message("Invalid payment id");
	// }
	if (paymentID_toUseOrToNilIfIntegrated != boost::none && paymentID_toUseOrToNilIfIntegrated->empty() == false) { // short pid / integrated address coersion
		if (decode_retVals.isSubaddress != true) { // this is critical or funds will be lost!!
			if (paymentID_toUseOrToNilIfIntegrated->size() == monero_paymentID_utils::payment_id_length__short) { // a short one
				THROW_WALLET_EXCEPTION_IF(decode_retVals.isSubaddress, error::wallet_internal_error, "Expected !decode_retVals.isSubaddress"); // just an extra safety measure
				optional<string> fabricated_integratedAddress_orNone = monero::address_utils::new_integratedAddrFromStdAddr( // construct integrated address
					xmrAddress_toDecode, // the monero one
					*paymentID_toUseOrToNilIfIntegrated, // short pid
					this->parameters.nettype
				);
				if (fabricated_integratedAddress_orNone == boost::none) {
					return error_ret_json_from_message("Could not construct integrated address");
				}
				this->to_address_string = *fabricated_integratedAddress_orNone;
				this->payment_id_string = boost::none; // must now zero this or Send will throw a "pid must be blank with integrated addr"
				this->isXMRAddressIntegrated = true;
				this->integratedAddressPIDForDisplay = *paymentID_toUseOrToNilIfIntegrated; // a short pid
				//this->_proceedTo_generateSendTransaction();
				// return early to prevent fall-through to non-short or zero pid case
				boost::property_tree::ptree root;
				root.put("retVal", "true");
			
				return ret_json_from_root(root);
			}
		}
	}
	this->to_address_string = xmrAddress_toDecode; // therefore, non-integrated normal XMR address
	this->payment_id_string = paymentID_toUseOrToNilIfIntegrated; // may still be nil
	this->isXMRAddressIntegrated = false;
	this->integratedAddressPIDForDisplay = boost::none;
	//this->_proceedTo_generateSendTransaction();
	
	const bool step1 = this->cb_I__got_unspent_outs(this->parameters.unspentOuts);
	if (!step1) {
		return error_ret_json_from_message(this->failureReason);
	}

	const bool reenter = this->_reenterable_construct_and_send_tx();
	if (!reenter) {
		return error_ret_json_from_message(this->failureReason);
	}
	auto req_params = new__req_params__get_random_outs(this->step1_retVals__using_outs); // use the one on the heap, since we've moved the one from step1_retVals
	// this->randomOuts = this->get_random_outs(req_params);

	boost::property_tree::ptree req_params_root;
		boost::property_tree::ptree amounts_ptree;
		BOOST_FOREACH(const string &amount_string, req_params.amounts)
		{
			property_tree::ptree amount_child;
			amount_child.put("", amount_string);
			amounts_ptree.push_back(std::make_pair("", amount_child));
		}
		req_params_root.add_child("amounts", amounts_ptree);
		req_params_root.put("count", req_params.count);
		stringstream req_params_ss;
		boost::property_tree::write_json(req_params_ss, req_params_root, false/*pretty*/);
	
	return req_params_ss.str().c_str();
}
void FormSubmissionController::_proceedTo_generateSendTransaction()
{
	this->cb_I__got_unspent_outs(this->parameters.unspentOuts);
}
bool FormSubmissionController::cb_I__got_unspent_outs(const optional<property_tree::ptree> &res)
{
	crypto::secret_key sec_viewKey{};
	crypto::secret_key sec_spendKey{};
	crypto::public_key pub_spendKey{};
	{
		bool r = false;
		r = epee::string_tools::hex_to_pod(this->parameters.sec_viewKey_string, sec_viewKey);
		if (!r) {
			this->failureReason = "Invalid privateViewKey";
			return false;
		}
		r = epee::string_tools::hex_to_pod(this->parameters.sec_spendKey_string, sec_spendKey);
		if (!r) {
			this->failureReason = "Invalid privateSpendKey";
			return false;
		}
		r = epee::string_tools::hex_to_pod(this->parameters.pub_spendKey_string, pub_spendKey);
		if (!r) {
			this->failureReason = "Invalid publicSpendKey";
			return false;
		}
	}
	auto parsed_res = new__parsed_res__get_unspent_outs(
		res.get(),
		sec_viewKey,
		sec_spendKey,
		pub_spendKey
	);
	if (parsed_res.err_msg != boost::none) {
		this->failureReason = std::move(*(parsed_res.err_msg));
		return false;
	}
	this->unspent_outs = std::move(*(parsed_res.unspent_outs));
	this->fee_per_b = *(parsed_res.per_byte_fee);
	this->fee_mask = *(parsed_res.fee_mask);
	this->use_fork_rules = monero_fork_rules::make_use_fork_rules_fn(parsed_res.fork_version);
	//
	this->passedIn_attemptAt_fee = boost::none;
	this->constructionAttempt = 0;

	return true;
}
bool FormSubmissionController::_reenterable_construct_and_send_tx()
{
	Send_Step1_RetVals step1_retVals;
	monero_transfer_utils::send_step1__prepare_params_for_get_decoys(
		step1_retVals,
		//
		this->payment_id_string,
		this->sending_amount,
		this->parameters.is_sweeping,
		this->parameters.priority,
		this->use_fork_rules,
		this->unspent_outs,
		this->fee_per_b,
		this->fee_mask,
		//
		this->passedIn_attemptAt_fee // use this for passing step2 "must-reconstruct" return values back in, i.e. re-entry; when none, defaults to attempt at network min
		// ^- and this will be 'none' as initial value
	);
	if (step1_retVals.errCode != noError) {
		this->failureReason = "Not enough spendables";
		return false;
	}
	// now store step1_retVals for step2
	this->step1_retVals__final_total_wo_fee = step1_retVals.final_total_wo_fee;
	this->step1_retVals__using_fee = step1_retVals.using_fee;
	this->step1_retVals__change_amount = step1_retVals.change_amount;
	this->step1_retVals__mixin = step1_retVals.mixin;
	if (this->step1_retVals__using_outs.size() != 0) {
		this->failureReason = "Expected 0 using_outs";
		return false;
	}
	this->step1_retVals__using_outs = std::move(step1_retVals.using_outs); // move structs from stack's vector to heap's vector
	this->valsState = WAIT_FOR_STEP2;
	
	return true;
}
bool FormSubmissionController::cb_II__got_random_outs(const optional<property_tree::ptree> &res) {
	auto parsed_res = new__parsed_res__get_random_outs(res.get());
	if (parsed_res.err_msg != boost::none) {
		this->failureReason = std::move(*(parsed_res.err_msg));
		return false;
	}
	if (parsed_res.err_msg != boost::none) {
		this->failureReason = "Expected non-0 using_outs";
		return false;
	}
	Send_Step2_RetVals step2_retVals;
	uint64_t unlock_time = 0; // hard-coded for now since we don't ever expose it, presently
	monero_transfer_utils::send_step2__try_create_transaction(
		step2_retVals,
		//
		this->parameters.from_address_string,
		this->parameters.sec_viewKey_string,
		this->parameters.sec_spendKey_string,
		this->to_address_string,
		this->payment_id_string,
		*(this->step1_retVals__final_total_wo_fee),
		*(this->step1_retVals__change_amount),
		*(this->step1_retVals__using_fee),
		this->parameters.priority,
		this->step1_retVals__using_outs,
		this->fee_per_b,
		this->fee_mask,
		*(parsed_res.mix_outs),
		this->use_fork_rules,
		unlock_time,
		this->parameters.nettype
	);
	if (step2_retVals.errCode != noError) {
		this->failureReason = "No balances";
		return false;
	}
	if (step2_retVals.tx_must_be_reconstructed) {
		// this will update status back to .calculatingFee
		if (this->constructionAttempt > 15) { // just going to avoid an infinite loop here or particularly long stack
			this->failureReason = "Exceeded construction attempts";
			return false;
		}
		this->valsState = WAIT_FOR_STEP1; // must reset this
		//
		this->constructionAttempt += 1; // increment for re-entry
		this->passedIn_attemptAt_fee = step2_retVals.fee_actually_needed; // -> reconstruction attempt's step1's passedIn_attemptAt_fee
		// reset step1 vals for correctness: (otherwise we end up, for example, with duplicate outs added)
		this->step1_retVals__final_total_wo_fee = none;
		this->step1_retVals__change_amount = none;
		this->step1_retVals__using_fee = none;
		this->step1_retVals__mixin = none;
		this->step1_retVals__using_outs.clear(); // critical!
		// and let's reset step2 just for clarity/explicitness, though we don't expect them to have values yet:
		this->step2_retVals__signed_serialized_tx_string = boost::none;
		this->step2_retVals__tx_hash_string = boost::none;
		this->step2_retVals__tx_key_string = boost::none;
		this->step2_retVals__tx_pub_key_string = boost::none;
		//
		_reenterable_construct_and_send_tx();
		return false;
	}
	// move step2 vals onto heap for later:
	this->step2_retVals__signed_serialized_tx_string = *(step2_retVals.signed_serialized_tx_string);
	this->step2_retVals__tx_hash_string = *(step2_retVals.tx_hash_string);
	this->step2_retVals__tx_key_string = *(step2_retVals.tx_key_string);
	this->step2_retVals__tx_pub_key_string = *(step2_retVals.tx_pub_key_string);
	//
	this->valsState = WAIT_FOR_FINISH;
	//
	
	return true;
}

string FormSubmissionController::cb_III__submitted_tx()
{
	// not actually expecting anything in a success response, so no need to parse it
	//
	Success_RetVals success_retVals;
	success_retVals.used_fee = *(this->step1_retVals__using_fee); // NOTE: not the same thing as step2_retVals.fee_actually_needed
	success_retVals.total_sent = *(this->step1_retVals__final_total_wo_fee) + *(this->step1_retVals__using_fee);
	success_retVals.mixin = *(this->step1_retVals__mixin);
	{
		optional<string> returning__payment_id = this->payment_id_string; // separated from submit_raw_tx_fn so that it can be captured w/o capturing all of args
		if (returning__payment_id == boost::none) {
			auto decoded = monero::address_utils::decodedAddress(this->to_address_string, this->parameters.nettype);
			if (decoded.did_error) { // would be very strange...
				return error_ret_json_from_message("Invalid destinationAddress");
			}
			if (decoded.paymentID_string != boost::none) {
				returning__payment_id = std::move(*(decoded.paymentID_string)); // just preserving this as an original return value - this can probably eventually be removed
			}
		}
		success_retVals.final_payment_id = returning__payment_id;
	}
	success_retVals.signed_serialized_tx_string = *(this->step2_retVals__signed_serialized_tx_string);
	success_retVals.tx_hash_string = *(this->step2_retVals__tx_hash_string);
	success_retVals.tx_key_string = *(this->step2_retVals__tx_key_string);
	success_retVals.tx_pub_key_string = *(this->step2_retVals__tx_pub_key_string);
	success_retVals.final_total_wo_fee = *(this->step1_retVals__final_total_wo_fee);
	success_retVals.isXMRAddressIntegrated = this-isXMRAddressIntegrated;
	success_retVals.integratedAddressPIDForDisplay = this->integratedAddressPIDForDisplay;
	success_retVals.target_address = this->to_address_string;
	//
	// this->parameters.success_fn(success_retVals);

	boost::property_tree::ptree root;
	root.put(ret_json_key__send__used_fee(), std::move(RetVals_Transforms::str_from(success_retVals.used_fee)));
	root.put(ret_json_key__send__total_sent(), std::move(RetVals_Transforms::str_from(success_retVals.total_sent)));
	root.put(ret_json_key__send__mixin(), success_retVals.mixin); // this is a uint32 so it can be sent in JSON
	if (success_retVals.final_payment_id) {
		root.put(ret_json_key__send__final_payment_id(), std::move(*(success_retVals.final_payment_id)));
	}
	root.put(ret_json_key__send__serialized_signed_tx(), std::move(success_retVals.signed_serialized_tx_string));
	root.put(ret_json_key__send__tx_hash(), std::move(success_retVals.tx_hash_string));
	root.put(ret_json_key__send__tx_key(), std::move(success_retVals.tx_key_string));
	root.put(ret_json_key__send__tx_pub_key(), std::move(success_retVals.tx_pub_key_string));
	root.put("target_address", std::move(success_retVals.target_address));
	root.put("final_total_wo_fee", std::move(RetVals_Transforms::str_from(success_retVals.final_total_wo_fee)));
	root.put("isXMRAddressIntegrated", std::move(RetVals_Transforms::str_from(success_retVals.isXMRAddressIntegrated)));
	if (success_retVals.integratedAddressPIDForDisplay) {
		root.put("integratedAddressPIDForDisplay", std::move(*(success_retVals.integratedAddressPIDForDisplay)));
	}

	return ret_json_from_root(root).c_str();
}
