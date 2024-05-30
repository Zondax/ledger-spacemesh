/*******************************************************************************
*   (c) 2018 - 2024 Zondax AG
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#include "gmock/gmock.h"

#include <vector>
#include <iostream>
#include <hexutils.h>
#include "address_encoding.h"
#include "parser_txdef.h"
#include "bech32.h"

using namespace std;

string toHexString(const uint8_t* data, size_t length) {
    std::stringstream hexStream;
    hexStream << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        hexStream << std::setw(2) << static_cast<int>(data[i]);
    }
    return hexStream.str();
}

TEST(Keys, WalletAddressEncoding) {
    for (const auto& testcase : testvectorWallet) {
        const string prefix = "stest";
        const bool isTestNet = testcase.address.substr(0, prefix.size()) == prefix;
        account_t walletccount{};

        // Read pubkey from testvectors and set up wallet account
        parseHexString(walletccount.keys[0], PUB_KEY_LENGTH, testcase.publicKey.c_str());
        walletccount.participants = 1;
        walletccount.id = WALLET;

        uint8_t address[64] = {0};
        uint8_t offset = 0;
        crypto_encodeAccountPubkey(address, sizeof(address), &walletccount, &offset, !isTestNet);
        char addressBench32[64] = {0};
        const char* hrp = isTestNet ? "stest" : "sm";
        bech32EncodeFromBytes(addressBench32, sizeof(addressBench32), hrp, address + offset, ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32);

        std::string str(reinterpret_cast<const char*>(addressBench32), testcase.address.size());
        EXPECT_EQ(str, testcase.address);
    }
}

TEST(Keys, MultisigAddressEncoding) {
    for (const auto& testcase : testvectorMultisig) {
        const string prefix = "stest";
        const bool isTestNet = testcase.address.substr(0, prefix.size()) == prefix;
        account_t multisigAccount{};

        // Read pubkeys from testvectors and set up multisig account
        for (auto i = 0; i < testcase.publicKeys.size(); i++){
            parseHexString(multisigAccount.keys[i], PUB_KEY_LENGTH, testcase.publicKeys[i].c_str());
        }
        multisigAccount.participants = testcase.participants;
        multisigAccount.approvers = testcase.approvals;
        multisigAccount.id = MULTISIG;

        
        uint8_t address[64] = {0};
        uint8_t offset = 0;
        crypto_encodeAccountPubkey(address, sizeof(address), &multisigAccount, &offset, !isTestNet);\
        char addressBench32[64] = {0};
        const char* hrp = isTestNet ? "stest" : "sm";
        bech32EncodeFromBytes(addressBench32, sizeof(addressBench32), hrp, address + offset, ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32);

        std::string str(reinterpret_cast<const char*>(addressBench32), testcase.address.size());
        EXPECT_EQ(str, testcase.address);
    }
}

TEST(Keys, VestingAddressEncoding) {
    for (const auto& testcase : testvectorVesting) {
        const string prefix = "stest";
        const bool isTestNet = testcase.address.substr(0, prefix.size()) == prefix;
        account_t vestingAccount{};

        // Read pubkeys from testvectors and set up vesting account
        for (auto i = 0; i < testcase.publicKeys.size(); i++){
            parseHexString(vestingAccount.keys[i], PUB_KEY_LENGTH, testcase.publicKeys[i].c_str());
        }
        vestingAccount.participants = testcase.participants;
        vestingAccount.approvers = testcase.approvals;
        vestingAccount.id = VESTING;
        
        uint8_t address[64] = {0};
        uint8_t offset = 0;
        crypto_encodeAccountPubkey(address, sizeof(address), &vestingAccount, &offset, !isTestNet);
        char addressBench32[64] = {0};
        const char* hrp = isTestNet ? "stest" : "sm";
        bech32EncodeFromBytes(addressBench32, sizeof(addressBench32), hrp, address + offset, ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32);

        std::string str(reinterpret_cast<const char*>(addressBench32), testcase.address.size());
        EXPECT_EQ(str, testcase.address);
    }
}

TEST(Keys, VaultAddressEncoding) {
    for (const auto& testcase : testvectorVault) {
        const string prefix = "stest";
        const bool isTestNet = testcase.address.substr(0, prefix.size()) == prefix;
        vault_account_t vaultAccount{};

        // Read pubkeys from testvectors and set up owner account in vault account
        for (auto i = 0; i < testcase.owner.publicKeys.size(); i++){
            parseHexString(vaultAccount.owner.keys[i], 32, testcase.owner.publicKeys[i].c_str());
        }
        vaultAccount.owner.participants = testcase.owner.participants;
        vaultAccount.owner.approvers = testcase.owner.approvals;
        vaultAccount.owner.id = VESTING;

        // set up vault account
        vaultAccount.totalAmount = testcase.totalAmount;
        vaultAccount.initialUnlockAmount = testcase.initialUnlockAmount;
        vaultAccount.vestingStart = testcase.vestingStart;
        vaultAccount.vestingEnd = testcase.vestingEnd;
        vaultAccount.id = VAULT;
        
        uint8_t address[64] = {0};
        uint8_t offset = 0;
        crypto_encodeVaultPubkey(address, sizeof(address), &vaultAccount, &offset, !isTestNet);
        char addressBench32[64] = {0};
        const char* hrp = isTestNet ? "stest" : "sm";
        bech32EncodeFromBytes(addressBench32, sizeof(addressBench32), hrp, address + offset, ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32);

        std::string str(reinterpret_cast<const char*>(addressBench32), testcase.address.size());
        EXPECT_EQ(str, testcase.address);
    }
}
