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

#include "coin.h"
#include "crypto.h"
#include "parser_common.h"
#include "parser_impl.h"
#include "zxblake3.h"

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
                    *num_items = 6;
                    break;
                case MULTISIG:
                case VESTING:
                    *num_items = 6 + ctx->tx_obj->spawn.multisig.numberOfPubkeys;
                    break;
                case VAULT:
                    *num_items = 10;
                    break;
                default:
                    return parser_unexpected_value;
            }
            break;
        case METHOD_SPEND:
            *num_items = 6;
            break;
        case METHOD_DRAIN_VAULT:
            *num_items = 7;
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
    char buff[50] = {0};
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            snprintf(outVal, outKeyLen, "Spend");
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->principal.ptr), ADDRESS_LENGTH, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Destination");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->spend.destination.ptr), ADDRESS_LENGTH, pageIdx,
                          pageCount);
            break;
        case 3:
            snprintf(outKey, outKeyLen, "Amount");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->spend.amount) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;

        case 4:
            snprintf(outKey, outKeyLen, "Gas price");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->gas_price) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 5:
            snprintf(outKey, outKeyLen, "Nonce");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->nonce) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        default:
            return parser_no_data;
    }

    return parser_ok;
}

parser_error_t printWalletSpawn(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                                char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    char buff[50] = {0};
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            snprintf(outVal, outKeyLen, "Wallet spawn");
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->principal.ptr), ADDRESS_LENGTH, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Pubkey");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->spawn.wallet.pubkey.ptr), PUB_KEY_LENGTH, pageIdx,
                          pageCount);
            break;
        case 3:
            snprintf(outKey, outKeyLen, "Account template");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->spawn.account_template.ptr), ADDRESS_LENGTH,
                          pageIdx, pageCount);
            break;

        case 4:
            snprintf(outKey, outKeyLen, "Gas price");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->gas_price) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 5:
            snprintf(outKey, outKeyLen, "Nonce");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->nonce) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        default:
            return parser_no_data;
    }

    return parser_ok;
}

parser_error_t printMultisigSpawn(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                                  char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    char buff[50] = {0};
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
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->principal.ptr), ADDRESS_LENGTH, pageIdx, pageCount);
            return parser_ok;
        case 2:
            snprintf(outKey, outKeyLen, "Account template");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->spawn.account_template.ptr), ADDRESS_LENGTH,
                          pageIdx, pageCount);
            break;

        case 3:
            snprintf(outKey, outKeyLen, "Gas price");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->gas_price) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 4:
            snprintf(outKey, outKeyLen, "Nonce");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->nonce) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 5:
            snprintf(outKey, outKeyLen, "Validators");
            snprintf(outVal, outValLen, "%d", ctx->tx_obj->spawn.multisig.approvers);
            break;
        default: {
            const uint8_t tmpDisplayIdx = displayIdx - 6;
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
    char buff[50] = {0};
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            snprintf(outVal, outKeyLen, "Vault spawn");
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->principal.ptr), ADDRESS_LENGTH, pageIdx, pageCount);
            return parser_ok;
        case 2:
            snprintf(outKey, outKeyLen, "Account template");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->spawn.account_template.ptr), ADDRESS_LENGTH,
                          pageIdx, pageCount);
            break;

        case 3:
            snprintf(outKey, outKeyLen, "Gas price");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->gas_price) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 4:
            snprintf(outKey, outKeyLen, "Nonce");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->nonce) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 5:
            snprintf(outKey, outKeyLen, "Owner");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->spawn.vault.owner.ptr), ADDRESS_LENGTH, pageIdx,
                          pageCount);
            break;
        case 6:
            // TODO: should we show decimals? same for cases 7, 8 and 9
            snprintf(outKey, outKeyLen, "TotalAmount");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->spawn.vault.totalAmount) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 7:
            snprintf(outKey, outKeyLen, "InitialUnlockAmount");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->spawn.vault.initialUnlockAmount) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 8:
            snprintf(outKey, outKeyLen, "VestingStart");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->spawn.vault.vestingStart) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 9:
            snprintf(outKey, outKeyLen, "VestingEnd");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->spawn.vault.vestingEnd) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        default: {
            return parser_no_data;
        }
    }

    return parser_ok;
}

parser_error_t printDrainTx(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal,
                            uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    char buff[50] = {0};
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Tx type");
            snprintf(outVal, outKeyLen, "Vault drain");
            break;
        case 1:
            snprintf(outKey, outKeyLen, "Principal");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->principal.ptr), ADDRESS_LENGTH, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Destination");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->drain.destination.ptr), ADDRESS_LENGTH, pageIdx,
                          pageCount);
            break;
        case 3:
            snprintf(outKey, outKeyLen, "Vault");
            pageStringHex(outVal, outValLen, (const char *)(ctx->tx_obj->drain.vault.ptr), ADDRESS_LENGTH, pageIdx,
                          pageCount);
            break;
        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->drain.amount) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;

        case 5:
            snprintf(outKey, outKeyLen, "Gas price");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->gas_price) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        case 6:
            snprintf(outKey, outKeyLen, "Nonce");
            if (uint64_to_str(buff, sizeof(buff), ctx->tx_obj->nonce) != NULL) {
                return parser_unexpected_error;
            }
            pageString(outVal, outValLen, buff, pageIdx, pageCount);
            break;
        default:
            return parser_no_data;
    }

    return parser_ok;
}