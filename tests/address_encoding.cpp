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
#include "address_encoding.h"

#include <hexutils.h>

#include <iostream>
#include <vector>

#include "bech32.h"
#include "gmock/gmock.h"
#include "parser_txdef.h"

using namespace std;

string toHexString(const uint8_t *data, size_t length) {
    std::stringstream hexStream;
    hexStream << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        hexStream << std::setw(2) << static_cast<int>(data[i]);
    }
    return hexStream.str();
}

TEST(Keys, WalletAddressEncoding) {
    for (const auto &testcase : testvectorWallet) {
        const string prefix = "stest";
        const bool isTestNet = testcase.address.substr(0, prefix.size()) == prefix;
        pubkey_t internalPubkey{};

        // Read pubkey from testvectors and set up wallet account
        parseHexString(internalPubkey.pubkey, PUB_KEY_LENGTH, testcase.publicKey.c_str());

        uint8_t address[64] = {0};
        crypto_encodeAccountPubkey(address, sizeof(address), &internalPubkey, NULL, WALLET);
        char addressBench32[64] = {0};
        const char *hrp = isTestNet ? "stest" : "sm";
        bech32EncodeFromBytes(addressBench32, sizeof(addressBench32), hrp, address, ADDRESS_LENGTH, 1,
                              BECH32_ENCODING_BECH32);

        std::string str(reinterpret_cast<const char *>(addressBench32), testcase.address.size());
        EXPECT_EQ(str, testcase.address);
    }
}

TEST(Keys, MultisigAddressEncoding) {
    for (const auto &testcase : testvectorMultisig) {
        const string prefix = "stest";
        const bool isTestNet = testcase.address.substr(0, prefix.size()) == prefix;
        pubkey_t internalPubkey{};
        account_t multisigAccount{};

        // Read pubkeys from testvectors and set up multisig account
        multisigAccount.internalIndex = 0;
        uint8_t indexAux = 0;
        for (auto i = 0; i < testcase.publicKeys.size(); i++) {
            if (i == multisigAccount.internalIndex) {
                parseHexString(internalPubkey.pubkey, PUB_KEY_LENGTH, testcase.publicKeys[i].c_str());
            } else {
                parseHexString(multisigAccount.keys[indexAux].pubkey, PUB_KEY_LENGTH, testcase.publicKeys[i].c_str());
                indexAux++;
            }
        }
        multisigAccount.participants = testcase.participants;
        multisigAccount.approvers = testcase.approvals;

        uint8_t address[64] = {0};
        crypto_encodeAccountPubkey(address, sizeof(address), &internalPubkey, &multisigAccount, MULTISIG);
        char addressBench32[64] = {0};
        const char *hrp = isTestNet ? "stest" : "sm";
        bech32EncodeFromBytes(addressBench32, sizeof(addressBench32), hrp, address, ADDRESS_LENGTH, 1,
                              BECH32_ENCODING_BECH32);

        std::string str(reinterpret_cast<const char *>(addressBench32), testcase.address.size());
        EXPECT_EQ(str, testcase.address);
    }
}

TEST(Keys, VestingAddressEncoding) {
    for (const auto &testcase : testvectorVesting) {
        const string prefix = "stest";
        const bool isTestNet = testcase.address.substr(0, prefix.size()) == prefix;
        pubkey_t internalPubkey{};
        account_t vestingAccount{};

        // Read pubkeys from testvectors and set up vesting account
        vestingAccount.internalIndex = 0;
        uint8_t indexAux = 0;
        for (auto i = 0; i < testcase.publicKeys.size(); i++) {
            if (i == vestingAccount.internalIndex) {
                parseHexString(internalPubkey.pubkey, PUB_KEY_LENGTH, testcase.publicKeys[i].c_str());
            } else {
                parseHexString(vestingAccount.keys[indexAux].pubkey, PUB_KEY_LENGTH, testcase.publicKeys[i].c_str());
                indexAux++;
            }
        }
        vestingAccount.participants = testcase.participants;
        vestingAccount.approvers = testcase.approvals;

        uint8_t address[64] = {0};
        std::cout << "testcase.address: " << testcase.address << std::endl;
        crypto_encodeAccountPubkey(address, sizeof(address), &internalPubkey, &vestingAccount, VESTING);
        char addressBench32[64] = {0};
        const char *hrp = isTestNet ? "stest" : "sm";
        bech32EncodeFromBytes(addressBench32, sizeof(addressBench32), hrp, address, ADDRESS_LENGTH, 1,
                              BECH32_ENCODING_BECH32);

        std::string str(reinterpret_cast<const char *>(addressBench32), testcase.address.size());
        EXPECT_EQ(str, testcase.address);
    }
}

TEST(Keys, VaultAddressEncoding) {
    for (const auto &testcase : testvectorVault) {
        const string prefix = "stest";
        const bool isTestNet = testcase.address.substr(0, prefix.size()) == prefix;
        pubkey_t internalPubkey{};
        vault_account_t vaultAccount{};

        // Read pubkeys from testvectors and set up owner account in vault account
        vaultAccount.owner.internalIndex = 0;
        uint8_t indexAux = 0;
        for (auto i = 0; i < testcase.owner.publicKeys.size(); i++) {
            if (i == vaultAccount.owner.internalIndex) {
                parseHexString(internalPubkey.pubkey, PUB_KEY_LENGTH, testcase.owner.publicKeys[i].c_str());
            } else {
                parseHexString(vaultAccount.owner.keys[indexAux].pubkey, 32, testcase.owner.publicKeys[i].c_str());
                indexAux++;
            }
        }
        vaultAccount.owner.participants = testcase.owner.participants;
        vaultAccount.owner.approvers = testcase.owner.approvals;

        // set up vault account
        vaultAccount.totalAmount = testcase.totalAmount;
        vaultAccount.initialUnlockAmount = testcase.initialUnlockAmount;
        vaultAccount.vestingStart = testcase.vestingStart;
        vaultAccount.vestingEnd = testcase.vestingEnd;

        uint8_t address[64] = {0};
        crypto_encodeVaultPubkey(address, sizeof(address), &internalPubkey, &vaultAccount, !isTestNet);
        char addressBench32[64] = {0};
        const char *hrp = isTestNet ? "stest" : "sm";
        bech32EncodeFromBytes(addressBench32, sizeof(addressBench32), hrp, address, ADDRESS_LENGTH, 1,
                              BECH32_ENCODING_BECH32);

        std::string str(reinterpret_cast<const char *>(addressBench32), testcase.address.size());
        EXPECT_EQ(str, testcase.address);
    }
}
