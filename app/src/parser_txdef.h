/*******************************************************************************
 *  (c) 2018 - 2023 Zondax AG
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "crypto_helper.h"

#define TX_VERSION 0
#define METHOD_SPAWN 0
#define METHOD_SPEND 16
#define METHOD_DRAIN_VAULT 17

// {TODO}: Placeholder, replace with real txn structure
typedef struct {
    uint16_t len;
    const uint8_t *ptr;
} Bytes_t;

typedef struct {
    uint8_t len;
    const uint8_t *ptr;
} CompactInt_t;

typedef struct {
    Bytes_t destination;
    uint64_t amount;
} spend_tx_t;

typedef struct {
    Bytes_t pubkey;
} spawn_wallet_tx_t;

typedef struct {
    uint8_t approvers;
    uint8_t numberOfPubkeys;
    Bytes_t pubkey[10];
} spawn_multisig_tx_t;

typedef struct {
    Bytes_t owner;
    uint64_t totalAmount;
    uint64_t initialUnlockAmount;
    uint32_t vestingStart;
    uint32_t vestingEnd;
} spawn_vault_tx_t;

typedef struct {
    Bytes_t account_template;
    union {
        spawn_wallet_tx_t wallet;
        spawn_multisig_tx_t multisig;
        spawn_vault_tx_t vault;
    };
} spawn_tx_t;

typedef struct {
    Bytes_t vault;
    Bytes_t destination;
    uint64_t amount;
} drain_tx_t;

typedef struct {
    Bytes_t genesisId;
    account_type_e account_type;
    uint8_t tx_version;
    Bytes_t principal;
    uint8_t methodSelector;
    uint64_t nonce;
    uint64_t gas_price;
    union {
        spend_tx_t spend;
        spawn_tx_t spawn;
        drain_tx_t drain;
    };

} parser_tx_t;

#ifdef __cplusplus
}
#endif
