/*******************************************************************************
 *   (c) 2018 - 2023 Zondax AG
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
#include "crypto_helper.h"

#include <stdio.h>
#include <string.h>

#include "zxblake3.h"
#include "zxmacros.h"

#define ADDRESS_RESERVED_SPACE 4

#define MAX_UINT6 0x3F
#define MAX_UINT14 0x3FFF
#define MAX_UINT30 0x3FFFFFFF
#define CHECK_PARSER_OK(CALL)           \
    do {                                \
        parser_error_t __cx_err = CALL; \
        if (__cx_err != parser_ok) {    \
            return zxerr_unknown;       \
        }                               \
    } while (0)

#define CHECK_ZX_OK(CALL)           \
    do {                            \
        zxerr_t __cx_err = CALL;    \
        if (__cx_err != zxerr_ok) { \
            return __cx_err;        \
        }                           \
    } while (0)

static uint16_t scaleEncodeUint8(uint64_t v, uint8_t *out) {
    if (out == NULL) {
        return 0;
    }
    v = v << 2;
    out[0] = (uint8_t)v;
    return 1;
}

static uint16_t scaleEncodeUint16(uint64_t v, uint8_t *out) {
    if (out == NULL) {
        return 0;
    }
    v = v << 2 | 0b01;
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)(v >> 8);
    return 2;
}

static uint16_t scaleEncodeUint32(uint64_t v, uint8_t *out) {
    if (out == NULL) {
        return 0;
    }
    v = v << 2 | 0b10;
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
    return 4;
}

static uint16_t scaleEncodeBigUint(uint64_t v, uint8_t *out) {
    int leading_zeros = __builtin_clzll(v);
    uint16_t needed = 8 - leading_zeros / 8;
    out[0] = (uint8_t)((needed - 4) << 2 | 0b11);
    for (int i = 1; i <= needed; ++i) {
        out[i] = (uint8_t)v;
        v >>= 8;
    }
    return needed + 1;
}

static uint16_t scaleEncodeUint64(uint64_t v, uint8_t *out) {
    if (out == NULL) {
        return 0;
    }
    if (v <= MAX_UINT6) {
        return scaleEncodeUint8(v, out);
    } else if (v <= MAX_UINT14) {
        return scaleEncodeUint16(v, out);
    } else if (v <= MAX_UINT30) {
        return scaleEncodeUint32(v, out);
    }

    return scaleEncodeBigUint(v, out);
}

static zxerr_t updateScaleEncodedNumber(uint64_t num) {
    uint8_t encNum[8] = {0};
    size_t size = scaleEncodeUint64(num, encNum);
    CHECK_PARSER_OK(zxblake3_hash_update(encNum, size));
    return zxerr_ok;
}

zxerr_t crypto_encodeAccountPubkey(uint8_t *address, uint16_t addressLen, const pubkey_item_t *internalPubkey,
                                   const generic_account_t *account, account_type_e account_type) {
    if (address == NULL || internalPubkey == NULL || addressLen < MAX_ADDRESS_LENGTH) {
        return zxerr_no_data;
    }

    uint8_t template[ADDRESS_LENGTH] = {0};
    template[ADDRESS_LENGTH - 1] = account_type;

    CHECK_PARSER_OK(zxblake3_hash_init());
    CHECK_PARSER_OK(zxblake3_hash_update(template, sizeof(template)));

    if (account_type == WALLET) {
        CHECK_PARSER_OK(zxblake3_hash_update(internalPubkey->pubkey, PUB_KEY_LENGTH));
    } else {
        if (account == NULL) {
            return zxerr_no_data;
        }
        if (account->approvers > account->participants || account->approvers == 0 ||
            account->participants > MAX_MULTISIG_PUB_KEY) {
            return zxerr_invalid_crypto_settings;
        }
        // both approvers and participants <= 63, perform inline scale encoding
        const uint8_t scaleApprovers = account->approvers << 2;
        CHECK_PARSER_OK(zxblake3_hash_update(&scaleApprovers, 1));

        const uint8_t scaleParticipants = account->participants << 2;
        CHECK_PARSER_OK(zxblake3_hash_update(&scaleParticipants, 1));

        // encode pubkeys
        uint8_t indexAux = 0;
        for (uint8_t i = 0; i < account->participants; i++) {
            if (i == internalPubkey->index) {
                CHECK_PARSER_OK(zxblake3_hash_update(internalPubkey->pubkey, PUB_KEY_LENGTH));
            } else {
                CHECK_PARSER_OK(zxblake3_hash_update(account->keys[indexAux].pubkey, PUB_KEY_LENGTH));
                indexAux++;
            }
        }
    }

    CHECK_PARSER_OK(zxblake3_hash_finalize(address, addressLen));

    const uint8_t hashOffset = PUB_KEY_LENGTH - ADDRESS_LENGTH;
    MEMSET(address + hashOffset, 0, ADDRESS_RESERVED_SPACE);
    MEMMOVE(address, address + hashOffset, ADDRESS_LENGTH);

    return zxerr_ok;
}

zxerr_t crypto_encodeVaultPubkey(uint8_t *address, uint16_t addressLen, const pubkey_item_t *internalPubkey,
                                 const vault_account_t *vaultAccount) {
    if (address == NULL || vaultAccount == NULL || addressLen < MAX_ADDRESS_LENGTH) {
        return zxerr_no_data;
    }

    uint8_t template[ADDRESS_LENGTH] = {0};
    template[ADDRESS_LENGTH - 1] = VAULT;

    // first get vesting address without bench32Encode and clean encode buffer
    uint8_t addressVesting[100] = {0};
    CHECK_ZX_OK(
        crypto_encodeAccountPubkey(addressVesting, sizeof(addressVesting), internalPubkey, &vaultAccount->owner, VESTING));

    CHECK_PARSER_OK(zxblake3_hash_init());
    CHECK_PARSER_OK(zxblake3_hash_update(template, sizeof(template)));
    CHECK_PARSER_OK(zxblake3_hash_update(addressVesting, ADDRESS_LENGTH));

    CHECK_ZX_OK(updateScaleEncodedNumber(vaultAccount->totalAmount));
    CHECK_ZX_OK(updateScaleEncodedNumber(vaultAccount->initialUnlockAmount));
    CHECK_ZX_OK(updateScaleEncodedNumber(vaultAccount->vestingStart));
    CHECK_ZX_OK(updateScaleEncodedNumber(vaultAccount->vestingEnd));

    CHECK_PARSER_OK(zxblake3_hash_finalize(address, addressLen));

    uint8_t hashOffset = PUB_KEY_LENGTH - ADDRESS_LENGTH;
    MEMZERO(address + hashOffset, ADDRESS_RESERVED_SPACE);
    MEMMOVE(address, address + hashOffset, ADDRESS_LENGTH);

    return zxerr_ok;
}
