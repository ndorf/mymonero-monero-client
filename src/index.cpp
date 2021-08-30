//
//  index.cpp
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

#include <stdio.h>
#include <emscripten/bind.h>
#include <emscripten.h>

#include "serial_bridge_index.hpp"
#include "serial_bridge_utils.hpp"
#include "emscr_SendFunds_bridge.hpp"

string decodeAddress(const string address, const string nettype)
{
    return serial_bridge::decode_address(address, nettype);
}

bool isSubaddress(const string address, const string nettype)
{
    return serial_bridge::is_subaddress(address, nettype);
}

bool isIntegratedAddress(const string address, const string nettype)
{
    return serial_bridge::is_integrated_address(address, nettype);
}

string newIntegratedAddress(const string address, const string paymentId, const string nettype)
{
    return serial_bridge::new_integrated_address(address, paymentId, nettype);
}

string generatePaymentId()
{
    return serial_bridge::new_payment_id();
}

string generateWallet(const string localeLanguageCode, const string nettype)
{
    return serial_bridge::newly_created_wallet(localeLanguageCode, nettype);
}

bool compareMnemonics(const string mnemonicA, const string mnemonicB)
{
    return serial_bridge::are_equal_mnemonics(mnemonicA, mnemonicB);
}

string addressAndKeysFromSeed(const string seed,  const string nettype)
{
    return serial_bridge::address_and_keys_from_seed(seed, nettype);
}

string mnemonicFromSeed(const string seed, const string wordsetName)
{
    return serial_bridge::mnemonic_from_seed(seed, wordsetName);
}

string seedAndKeysFromMnemonic(const string mnemonic, const string nettype)
{
    return serial_bridge::seed_and_keys_from_mnemonic(mnemonic, nettype);
}

string isValidKeys(const string address, const string privateViewKey, const string privateSpendKey, const string seed, const string nettype)
{
    return serial_bridge::validate_components_for_login(address, privateViewKey, privateSpendKey, seed, nettype);
}

string estimateTxFee(const string priority, const string feePerb, const string forkVersion)
{
    return serial_bridge::estimated_tx_network_fee(priority, feePerb, forkVersion);
}

string generateKeyImage(const string txPublicKey, const string privateViewKey, const string publicSpendKey, const string privateSpendKey, const string outputIndex)
{
    return serial_bridge::generate_key_image(txPublicKey, privateViewKey, publicSpendKey, privateSpendKey, outputIndex);
}

string prepareSend(const string &args_string)
{
    return emscr_SendFunds_bridge::prepare_send(args_string);
}

string sendFunds(const string &args_string)
{
    return emscr_SendFunds_bridge::send_funds(args_string);
}

std::string getExceptionMessage(intptr_t exceptionPtr) {
  return std::string(reinterpret_cast<std::exception *>(exceptionPtr)->what());
}

EMSCRIPTEN_BINDINGS(my_module)
{ // C++ -> JS 
    emscripten::function("getExceptionMessage", &getExceptionMessage);
    emscripten::function("decodeAddress", &decodeAddress);
    emscripten::function("isSubaddress", &isSubaddress);
    emscripten::function("isIntegratedAddress", &isIntegratedAddress);

    emscripten::function("newIntegratedAddress", &newIntegratedAddress);
    emscripten::function("generatePaymentId", &generatePaymentId);

    emscripten::function("generateWallet", &generateWallet);
    emscripten::function("compareMnemonics", &compareMnemonics);
    emscripten::function("mnemonicFromSeed", &mnemonicFromSeed);
    emscripten::function("seedAndKeysFromMnemonic", &seedAndKeysFromMnemonic);
    emscripten::function("isValidKeys", &isValidKeys);
    emscripten::function("addressAndKeysFromSeed", &addressAndKeysFromSeed);

    emscripten::function("estimateTxFee", &estimateTxFee);

    emscripten::function("generateKeyImage", &generateKeyImage);
    emscripten::function("prepareTx", &prepareSend);
    emscripten::function("createAndSignTx", &sendFunds);
}
extern "C"
{ // C -> JS
}
int main() {
  // printf("hello, world!\n");
  return 0;
}
