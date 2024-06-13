/*******************************************************************************
 *  (c) 2018 - 2024 Zondax AG
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
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "coin.h"
#include "zxerror.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADDRESS_LENGTH 24
#define MIN_MAIN_ADDRESS_BUFFER_LEN 48
#define MIN_TEST_ADDRESS_BUFFER_LEN 51
#define MAX_MULTISIG_PUB_KEY 10

typedef enum {
    UNKNOWN = 0,
    WALLET = 1,
    MULTISIG = 2,
    VESTING = 3,
    VAULT = 4,
} account_type_e;

typedef struct {
    uint8_t index;
    uint8_t pubkey[32];
} __attribute__((packed)) pubkey_item_t;

typedef struct {
    uint8_t approvers;
    uint8_t participants;
    pubkey_item_t keys[MAX_MULTISIG_PUB_KEY];
} __attribute__((packed)) generic_account_t;

typedef struct {
    uint64_t totalAmount;
    uint64_t initialUnlockAmount;
    uint32_t vestingStart;
    uint32_t vestingEnd;
    generic_account_t owner;
} __attribute__((packed)) vault_account_t;

zxerr_t crypto_encodeAccountPubkey(uint8_t *address, uint16_t addressLen, const pubkey_item_t *internalPubkey,
                                   const generic_account_t *account, account_type_e id);
zxerr_t crypto_encodeVaultPubkey(uint8_t *address, uint16_t addressLen, const pubkey_item_t *internalPubkey,
                                 const vault_account_t *vaultAccount, bool mainnet);

#ifdef __cplusplus
}
#endif
