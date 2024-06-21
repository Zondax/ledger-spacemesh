/*******************************************************************************
 *  (c) 2018 - 2024  Zondax AG
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
#include "parser_impl.h"
#include "parser_txdef.h"
#include "scale_helper.h"

// GEN_DEF_READFIX_UNSIGNED(8)

// GEN_DEF_READFIX_UNSIGNED(16)

// GEN_DEF_READFIX_UNSIGNED(32)

// GEN_DEF_READFIX_UNSIGNED(64)

parser_error_t readCompactInt(parser_context_t *ctx, CompactInt_t *val) {
    CHECK_INPUT();

    val->ptr = ctx->buffer + ctx->offset;
    const uint8_t mode = *val->ptr & 0x03U;  // get mode from two least significant bits

    uint64_t tmp = {0};
    switch (mode) {
        case 0:  // single byte
            val->len = 1;
            CTX_CHECK_AND_ADVANCE(ctx, val->len)
            _getValue(val, &tmp);
            break;
        case 1:  // 2-byte
            val->len = 2;
            CTX_CHECK_AND_ADVANCE(ctx, val->len)
            _getValue(val, &tmp);
            break;
        case 2:  // 4-byte
            val->len = 4;
            CTX_CHECK_AND_ADVANCE(ctx, val->len)
            _getValue(val, &tmp);
            break;
        case 3:  // bigint
            val->len = (*val->ptr >> 2U) + 4 + 1;
            CTX_CHECK_AND_ADVANCE(ctx, val->len)
            break;
        default:
            // this is actually impossible
            return parser_unexpected_value;
    }

    return parser_ok;
}

parser_error_t _getValue(const CompactInt_t *compact, uint64_t *value) {
    if (compact == NULL || value == NULL) {
        return parser_no_data;
    }
    *value = 0;

    switch (compact->len) {
        case 1:
            *value = (*compact->ptr) >> 2U;
            break;
        case 2:
            *value = (*compact->ptr) >> 2U;
            *value += *(compact->ptr + 1) << 6U;
            if (*value < 64U) {
                return parser_value_out_of_range;
            }
            break;
        case 4:
            *value = (*compact->ptr) >> 2U;
            *value += *(compact->ptr + 1) << 6U;
            *value += *(compact->ptr + 2) << (8U + 6U);
            *value += *(compact->ptr + 3) << (16U + 6U);
            if (*value < 16383U) {
                return parser_value_out_of_range;
            }
            break;
        default: {
            uint8_t upper_bits = (compact->ptr[0] & 0xFC);
            uint8_t byte_length = (upper_bits >> 2) + 4;
            if (byte_length > 8) {
                return parser_value_out_of_range;
            }
            *value = 0;
            for (int i = byte_length; i > 1; i--) {
                *value |= compact->ptr[i] & 0xFF;
                *value <<= 8;
            }
            *value |= compact->ptr[1] & 0xFF;

            if (*value > 4611686018427387903) {
                return parser_value_out_of_range;
            }
            break;
        }
    }
    return parser_ok;
}

parser_error_t _readTxVersion(parser_context_t *ctx, uint8_t *val) {
    CHECK_INPUT();
    CHECK_ERROR(readCompactU8(ctx, val));
    if (*val != TX_VERSION) {
        return parser_unexpected_version;
    }
    return parser_ok;
}

parser_error_t _readMethodSelector(parser_context_t *ctx, uint8_t *val) {
    CHECK_INPUT();
    CHECK_ERROR(readCompactU8(ctx, val));
    if (*val != METHOD_SPAWN && *val != METHOD_SPEND) {
        return parser_unexpected_method_selector;
    }
    return parser_ok;
}

parser_error_t _readSpawnTx(parser_context_t *ctx, parser_tx_t *val) {
    CHECK_INPUT();
    CHECK_ERROR(readFixedArray(ctx, &val->spawn.account_template, ADDRESS_LENGTH));
    CHECK_ERROR(readCompactU64(ctx, &val->nonce));
    CHECK_ERROR(readCompactU64(ctx, &val->gas_price));
    switch (val->spawn.account_template.ptr[val->spawn.account_template.len - 1]) {
        case WALLET:
            val->account_type = WALLET;
            CHECK_ERROR(readFixedArray(ctx, &val->spawn.wallet.pubkey, PUB_KEY_LENGTH));
            break;
        case MULTISIG:
        case VESTING:
            val->account_type = MULTISIG;
            CHECK_ERROR(readCompactU8(ctx, &val->spawn.multisig.approvers));
            if (val->spawn.multisig.approvers > MAX_MULTISIG_PUB_KEY) {
                return parser_unexpected_value;
            }
            CHECK_ERROR(readCompactU8(ctx, &val->spawn.multisig.numberOfPubkeys));
            if (val->spawn.multisig.numberOfPubkeys < val->spawn.multisig.approvers ||
                val->spawn.multisig.numberOfPubkeys > MAX_MULTISIG_PUB_KEY) {
                return parser_unexpected_value;
            }
            for (uint8_t i = 0; i < val->spawn.multisig.numberOfPubkeys; i++) {
                CHECK_ERROR(readFixedArray(ctx, &val->spawn.multisig.pubkey[i], PUB_KEY_LENGTH));
            }
            break;
        case VAULT:
            val->account_type = VAULT;
            CHECK_ERROR(readFixedArray(ctx, &val->spawn.vault.owner, ADDRESS_LENGTH));
            CHECK_ERROR(readCompactU64(ctx, &val->spawn.vault.totalAmount));
            CHECK_ERROR(readCompactU64(ctx, &val->spawn.vault.initialUnlockAmount));
            if (val->spawn.vault.initialUnlockAmount > val->spawn.vault.totalAmount) {
                return parser_unexpected_value;
            }
            CHECK_ERROR(readCompactU32(ctx, &val->spawn.vault.vestingStart));
            CHECK_ERROR(readCompactU32(ctx, &val->spawn.vault.vestingEnd));
            if (val->spawn.vault.vestingStart > val->spawn.vault.vestingEnd) {
                return parser_unexpected_value;
            }
            break;
    }
    return parser_ok;
}

parser_error_t _readSpendTx(parser_context_t *ctx, parser_tx_t *val) {
    CHECK_INPUT();
    val->account_type = WALLET;
    CHECK_ERROR(readCompactU64(ctx, &val->nonce));
    CHECK_ERROR(readCompactU64(ctx, &val->gas_price));
    CHECK_ERROR(readFixedArray(ctx, &val->spend.destination, ADDRESS_LENGTH));
    CHECK_ERROR(readCompactU64(ctx, &val->spend.amount));
    return parser_ok;
}
