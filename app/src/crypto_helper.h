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
#include "zxmacros.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ADDRESS_LENGTH 60
#define ADDRESS_LENGTH 24
#define MAX_MULTISIG_PUB_KEY 10

extern uint32_t hdPath[HDPATH_LEN_DEFAULT];

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

////////////

typedef struct {
    account_type_e account_type;
    uint8_t internalIndex;

    uint8_t optional_numberOfPubkeys;
    union {
        generic_account_t *account;
        vault_account_t *vault_account;
    };
} address_request_t;

/**
 * Calculate the human-readable part (hrp) for Bech32 encoding based on the network type.
 * @returns the appropriate hrp string for the network
 */
__Z_INLINE const char *calculate_hrp() {
    bool mainnet = hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT;
    return mainnet ? "sm" : "stest";
}

zxerr_t crypto_encodeAccountPubkey(uint8_t *address, uint16_t addressLen, const pubkey_item_t *internalPubkey,
                                   const generic_account_t *account, account_type_e id);
zxerr_t crypto_encodeVaultPubkey(uint8_t *address, uint16_t addressLen, const pubkey_item_t *internalPubkey,
                                 const vault_account_t *vaultAccount);

#ifdef __cplusplus
}
#endif
