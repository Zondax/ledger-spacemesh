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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <sigutils.h>
#include <stdbool.h>

#include "coin.h"
#include "crypto_helper.h"
#include "zxerror.h"

typedef struct {
    uint8_t pubkey[PUB_KEY_LENGTH];
    char address_bech32[MAX_ADDRESS_LENGTH];
} apdu_address_response_t;

zxerr_t crypto_fillAddress(uint8_t *outBuffer, uint16_t outBufferLen, uint16_t *addrResponseLen);
zxerr_t crypto_fillAddressMultisigOrVesting(uint8_t *outBuffer, uint16_t outBufferLen, uint16_t *addrResponseLen);
zxerr_t crypto_fillAddressVault(uint8_t *outBuffer, uint16_t outBufferLen, uint16_t *addrResponseLen);

zxerr_t crypto_sign(uint8_t *signature, uint16_t signatureMaxlen, const uint8_t *message, uint16_t messageLen);

#ifdef __cplusplus
}
#endif
