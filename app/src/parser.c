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

#include "parser.h"

#include <stdio.h>
#include <zxformat.h>
#include <zxmacros.h>
#include <zxtypes.h>

#include "app_mode.h"
#include "bech32.h"
#include "coin.h"
#include "parser_common.h"
#include "parser_impl.h"
#include "zxblake3.h"

#define CHECK_ZX_OK(CALL)                   \
    do {                                    \
        zxerr_t __cx_err = CALL;            \
        if (__cx_err != zxerr_ok) {         \
            return parser_unexpected_error; \
        }                                   \
    } while (0)

uint32_t hdPath[HDPATH_LEN_DEFAULT];

parser_error_t parser_init_context(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize) {
    ctx->offset = 0;
    ctx->buffer = NULL;
    ctx->bufferLen = 0;

    if (bufferSize == 0 || buffer == NULL) {
        // Not available, use defaults
        return parser_init_context_empty;
    }

    ctx->buffer = buffer;
    ctx->bufferLen = bufferSize;
    return parser_ok;
}

parser_error_t parser_parse(parser_context_t *ctx, const uint8_t *data, size_t dataLen, parser_tx_t *tx_obj) {
    CHECK_ERROR(parser_init_context(ctx, data, dataLen));
    ctx->tx_obj = tx_obj;
    return _read(ctx, tx_obj);
}

parser_error_t parser_validate(parser_context_t *ctx) {
    // Iterate through all items to check that all can be shown and are valid
    uint8_t numItems = 0;
    CHECK_ERROR(parser_getNumItems(ctx, &numItems));

    char tmpKey[40] = {0};
    char tmpVal[40] = {0};

    for (uint8_t idx = 0; idx < numItems; idx++) {
        uint8_t pageCount = 0;
        CHECK_ERROR(parser_getItem(ctx, idx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), 0, &pageCount));
    }
    return parser_ok;
}

parser_error_t parser_getNumItems(const parser_context_t *ctx, uint8_t *num_items) {
    *num_items = 0;

    if (ctx->tx_obj == NULL) {
        return parser_tx_obj_empty;
    }

    switch (ctx->tx_obj->methodSelector) {
        case METHOD_SPAWN:
            switch (ctx->tx_obj->account_type) {
                case WALLET:
                    *num_items = 3;
                    if (app_mode_expert()) {
                        *num_items = 6;
                    }
                    break;
                case MULTISIG:
                case VESTING:
                    *num_items = 3;
                    if (app_mode_expert()) {
                        *num_items = 8 + ctx->tx_obj->spawn.multisig.numberOfPubkeys;
                    }
                    break;
                case VAULT:
                    *num_items = 3;
                    if (app_mode_expert()) {
                        *num_items = 11;
                    }
                    break;
                default:
                    return parser_unexpected_value;
            }
            break;
        case METHOD_SPEND:
            *num_items = 5;
            if (app_mode_expert()) {
                *num_items = 7;
            }
            break;
        case METHOD_DRAIN_VAULT:
            *num_items = 6;
            if (app_mode_expert()) {
                *num_items = 8;
            }
            break;
        default:
            return parser_unexpected_value;
    }

    if (*num_items == 0) {
        return parser_unexpected_number_items;
    }
    return parser_ok;
}

static void cleanOutput(char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen) {
    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);
    snprintf(outKey, outKeyLen, "?");
    snprintf(outVal, outValLen, " ");
}

static parser_error_t checkSanity(uint8_t numItems, uint8_t displayIdx) {
    if (displayIdx >= numItems) {
        return parser_display_idx_out_of_range;
    }
    return parser_ok;
}

parser_error_t parser_getItem(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    UNUSED(pageIdx);
    *pageCount = 1;
    uint8_t numItems = 0;
    CHECK_ERROR(parser_getNumItems(ctx, &numItems));
    CHECK_APP_CANARY()

    CHECK_ERROR(checkSanity(numItems, displayIdx));
    cleanOutput(outKey, outKeyLen, outVal, outValLen);

    return printTxnFields(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

parser_error_t printTxnFields(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    switch (ctx->tx_obj->methodSelector) {
        case METHOD_SPAWN:
            switch (ctx->tx_obj->account_type) {
                case WALLET:
                    return printWalletSpawn(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
                case MULTISIG:
                case VESTING:
                    return printMultisigSpawn(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
                case VAULT:
                    return printVaultSpawn(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
                default:
                    return parser_unexpected_value;
            }
        case METHOD_SPEND:
            return printSpendTx(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
        case METHOD_DRAIN_VAULT:
            return printDrainTx(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
        default:
            return parser_unexpected_value;
    }
}

parser_error_t printSpendTx(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal,
                            uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    char buff[64] = {0};
    const char *hrp = calculate_hrp();
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            snprintf(outVal, outKeyLen, "Spend");
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->principal.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Destination");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->spend.destination.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 3:
            snprintf(outKey, outKeyLen, "Amount");
            return _printNumber(ctx->tx_obj->spend.amount, 0, "", "SMIDGE ", outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas price");
            return _printNumber(ctx->tx_obj->gas_price, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Nonce");
            return _printNumber(ctx->tx_obj->nonce, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Method");
            return _printNumber(ctx->tx_obj->methodSelector, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        default:
            return parser_no_data;
    }

    return parser_ok;
}

parser_error_t printWalletSpawn(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                                char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    char buff[64] = {0};
    const char *hrp = calculate_hrp();
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            snprintf(outVal, outKeyLen, "Wallet spawn");
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->principal.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Gas price");
            return _printNumber(ctx->tx_obj->gas_price, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Template");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->spawn.account_template.ptr,
                                              ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 4:
            snprintf(outKey, outKeyLen, "Nonce");
            return _printNumber(ctx->tx_obj->nonce, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Method");
            return _printNumber(ctx->tx_obj->methodSelector, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        default:
            return parser_no_data;
    }

    return parser_ok;
}

parser_error_t printMultisigSpawn(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                                  char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    char buff[64] = {0};
    const char *hrp = calculate_hrp();
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            if (ctx->tx_obj->account_type == MULTISIG) {
                snprintf(outVal, outKeyLen, "Multisig spawn");
            } else {
                snprintf(outVal, outKeyLen, "Vesting spawn");
            }
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->principal.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Gas price");
            return _printNumber(ctx->tx_obj->gas_price, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Template");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->spawn.account_template.ptr,
                                              ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;

        case 4:
            snprintf(outKey, outKeyLen, "Nonce");
            return _printNumber(ctx->tx_obj->nonce, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Method");
            return _printNumber(ctx->tx_obj->methodSelector, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Participants");
            snprintf(outVal, outValLen, "%d", ctx->tx_obj->spawn.multisig.numberOfPubkeys);
            break;
        case 7:
            snprintf(outKey, outKeyLen, "Validators");
            snprintf(outVal, outValLen, "%d", ctx->tx_obj->spawn.multisig.approvers);
            break;
        default: {
            const uint8_t tmpDisplayIdx = displayIdx - 8;
            snprintf(outKey, outKeyLen, "Pubkey %d", tmpDisplayIdx);
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->spawn.multisig.pubkey[tmpDisplayIdx].ptr),
                          PUB_KEY_LENGTH, pageIdx, pageCount);
            break;
        }
    }

    return parser_ok;
}

parser_error_t printVaultSpawn(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                               char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    char buff[64] = {0};
    const char *hrp = calculate_hrp();
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            snprintf(outVal, outKeyLen, "Vault spawn");
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->principal.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Gas price");
            return _printNumber(ctx->tx_obj->gas_price, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Template");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->spawn.account_template.ptr,
                                              ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;

        case 4:
            snprintf(outKey, outKeyLen, "Nonce");
            return _printNumber(ctx->tx_obj->nonce, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Method");
            return _printNumber(ctx->tx_obj->methodSelector, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Owner");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->spawn.vault.owner.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 7:
            // TODO: should we show decimals? same for cases 7, 8 and 9
            snprintf(outKey, outKeyLen, "TotalAmount");
            return _printNumber(ctx->tx_obj->spawn.vault.totalAmount, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "InitialUnlockAmount");
            return _printNumber(ctx->tx_obj->spawn.vault.initialUnlockAmount, 0, "", "", outVal, outValLen, pageIdx,
                                pageCount);
        case 9:
            snprintf(outKey, outKeyLen, "VestingStart");
            return _printNumber((uint64_t)ctx->tx_obj->spawn.vault.vestingStart, 0, "", "", outVal, outValLen, pageIdx,
                                pageCount);
        case 10:
            snprintf(outKey, outKeyLen, "VestingEnd");
            return _printNumber((uint64_t)ctx->tx_obj->spawn.vault.vestingEnd, 0, "", "", outVal, outValLen, pageIdx,
                                pageCount);
        default: {
            return parser_no_data;
        }
    }

    return parser_ok;
}

parser_error_t printDrainTx(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal,
                            uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    char buff[64] = {0};
    const char *hrp = calculate_hrp();
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            snprintf(outVal, outKeyLen, "Vesting drain");
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->principal.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Vault");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->drain.vault.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 3:
            snprintf(outKey, outKeyLen, "Destination");
            CHECK_ZX_OK(bech32EncodeFromBytes(buff, sizeof(buff), hrp, ctx->tx_obj->drain.destination.ptr, ADDRESS_LENGTH, 1,
                                              BECH32_ENCODING_BECH32));
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;

        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            return _printNumber(ctx->tx_obj->drain.amount, 0, "", "SMIDGE ", outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Gas price");
            return _printNumber(ctx->tx_obj->gas_price, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Nonce");
            return _printNumber(ctx->tx_obj->nonce, 0, "", "", outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Method");
            return _printNumber(ctx->tx_obj->methodSelector, 0, "", "", outVal, outValLen, pageIdx, pageCount);
            break;
        default:
            return parser_no_data;
    }

    return parser_ok;
}

parser_error_t _printNumber(uint64_t amount, uint8_t decimalPlaces, const char *postfix, const char *prefix, char *outValue,
                            uint16_t outValueLen, uint8_t pageIdx, uint8_t *pageCount) {
    char bufferUI[200] = {0};
    if (uint64_to_str(bufferUI, sizeof(bufferUI), amount) != NULL) {
        return parser_unexpected_value;
    }

    if (intstr_to_fpstr_inplace(bufferUI, sizeof(bufferUI), decimalPlaces) == 0) {
        return parser_unexpected_value;
    }

    if (z_str3join(bufferUI, sizeof(bufferUI), prefix, postfix) != zxerr_ok) {
        return parser_unexpected_buffer_end;
    }

    number_inplace_trimming(bufferUI, 1);

    pageString(outValue, outValueLen, bufferUI, pageIdx, pageCount);
    return parser_ok;
}
