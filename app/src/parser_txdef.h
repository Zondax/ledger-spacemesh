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

// {TODO}: Placeholder, replace with real txn structure
typedef struct {
    uint8_t txn_param_0;
    uint8_t txn_param_1;
    uint8_t txn_param_N;
} parser_tx_t;

typedef struct {
    uint8_t index;
    uint8_t pubkey[32];
}idx_pubkey_t;

typedef struct {
    uint8_t internalIndex;
    uint8_t approvers;
    uint8_t participants;
    idx_pubkey_t keys[10];
} multisig_t;

typedef struct {
    uint64_t totalAmount;
    uint64_t initialUnlockAmount;
    uint32_t vestingStart;
    uint32_t vestingEnd;
    uint8_t internalIndex;
    uint8_t approvers;
    uint8_t participants;
    idx_pubkey_t keys[10];
} __attribute__((packed)) vault_t;

#ifdef __cplusplus
}
#endif
