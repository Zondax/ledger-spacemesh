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

#include <stdint.h>
#include <stdbool.h>
#include "zxerror.h"
#ifdef __cplusplus
extern "C" {
#endif

#define PUB_KEY_LENGTH 32
#define ADDRESS_LENGTH 24
#define MIN_MAIN_ADDRESS_BUFFER_LEN 48
#define MIN_TEST_ADDRESS_BUFFER_LEN 51
#define MAX_MULTISIG_PUB_KEY 10

typedef enum  {
    WALLET = 1,
    MULTISIG = 2,
    VESTING = 3,
    VAULT = 4,
} account_type_e;

typedef uint8_t pubkey_t[PUB_KEY_LENGTH];
typedef struct {
    pubkey_t keys[MAX_MULTISIG_PUB_KEY];
    uint8_t participants;
    uint8_t approvers;
    account_type_e id;
} account_t;

typedef struct {
    account_t owner;
    uint64_t totalAmount;
    uint64_t initialUnlockAmount;
    uint32_t vestingStart;
    uint32_t vestingEnd;
    account_type_e id;
} vault_account_t;

zxerr_t crypto_encodeAccountPubkey(uint8_t *address, uint16_t addressLen, const account_t *account, uint8_t *buffOffset, bool mainnet);
zxerr_t crypto_encodeVaultPubkey(uint8_t *address, uint16_t addressLen, const vault_account_t *vaultAccount, uint8_t *buffOffset, bool mainnet);

#ifdef __cplusplus
}
#endif
