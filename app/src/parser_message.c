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
#include "parser_message.h"

#include <stdio.h>
#include <zxformat.h>

#include "scale_helper.h"

GEN_DEF_READFIX_UNSIGNED(8)
GEN_DEF_READFIX_UNSIGNED(16)

static void _formatOutput(const char *label, uint16_t len, const uint8_t *ptr, char *outKey, uint16_t outKeyLen,
                          char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    if (len == 0) {
        snprintf(outKey, outKeyLen, "%s", label);
        snprintf(outVal, outValLen, "Empty");
    } else {
        bool allPrintable = true;
        for (uint16_t i = 0; i < len; i++) {
            allPrintable &= IS_PRINTABLE(ptr[i]);
        }
        if (allPrintable) {
            snprintf(outKey, outKeyLen, "%s", label);
            pageStringExt(outVal, outValLen, (const char *)ptr, len, pageIdx, pageCount);
        } else {
            snprintf(outKey, outKeyLen, "%s (hex)", label);
            pageStringHex(outVal, outValLen, (const char *)ptr, len, pageIdx, pageCount);
        }
    }
}

static parser_error_t _readDomain(parser_context_t *ctx, uint8_t *val) {
    CHECK_INPUT();
    CHECK_ERROR(_readUInt8(ctx, val));

    switch (*val) {
        case ATX:
        case PROPOSAL:
        case BALLOT:
        case HARE:
        case POET:
        case BEACON_FIRST_MSG:
        case BEACON_FOLLOWUP_MSG:
            return parser_ok;
        default:
            return parser_unexpected_value;
    }
}

static const char *_domainToString(uint8_t val) {
    switch (val) {
        case ATX:
            return "ATX";
        case PROPOSAL:
            return "PROPOSAL";
        case BALLOT:
            return "BALLOT";
        case HARE:
            return "HARE";
        case POET:
            return "POET";
        case BEACON_FIRST_MSG:
            return "BEACON FIRST MSG";
        case BEACON_FOLLOWUP_MSG:
            return "BEACON FOLLOWUP MSG";
        default:
            return "UNKNOWN";
    }
}

static parser_error_t parser_message_checkSanity(uint8_t numItems, uint8_t displayIdx) {
    if (displayIdx >= numItems) {
        return parser_display_idx_out_of_range;
    }
    return parser_ok;
}

static void parser_message_cleanOutput(char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen) {
    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);
    snprintf(outKey, outKeyLen, "?");
    snprintf(outVal, outValLen, " ");
}

static parser_error_t parser_message_init_context(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize) {
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

static parser_error_t parser_message_read(parser_context_t *c, parser_message_tx_t *v) {
    // read common params
    if (c == NULL || v == NULL) {
        return parser_unexpected_error;
    }

    uint16_t prefix_len;
    uint16_t message_len;
    CHECK_ERROR(_readUInt16(c, &prefix_len));
    CHECK_ERROR(_readUInt16(c, &message_len));
    if (prefix_len > 0) {
        CHECK_ERROR(readFixedArray(c, &v->prefix, prefix_len));
    }
    CHECK_ERROR(_readDomain(c, &v->domain));
    if (message_len > 0) {
        CHECK_ERROR(readFixedArray(c, &v->message, message_len));
    }

    if (c->offset != c->bufferLen) {
        return parser_unexpected_unparsed_bytes;
    }

    return parser_ok;
}

parser_error_t parser_message_parse(parser_context_t *ctx, const uint8_t *data, size_t dataLen,
                                    parser_message_tx_t *tx_obj) {
    CHECK_ERROR(parser_message_init_context(ctx, data, dataLen));
    ctx->message_tx_obj = tx_obj;
    return parser_message_read(ctx, tx_obj);
}

parser_error_t parser_message_getNumItems(uint8_t *num_items) {
    *num_items = 4;
    return parser_ok;
}

parser_error_t parser_message_getItem(const parser_context_t *ctx, uint8_t displayIdx, char *outKey, uint16_t outKeyLen,
                                      char *outVal, uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
    UNUSED(pageIdx);
    *pageCount = 1;
    uint8_t numItems = 0;
    CHECK_ERROR(parser_message_getNumItems(&numItems));
    CHECK_APP_CANARY()

    CHECK_ERROR(parser_message_checkSanity(numItems, displayIdx));

    parser_message_cleanOutput(outKey, outKeyLen, outVal, outValLen);

    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Sign");
            snprintf(outVal, outValLen, "Message");
            break;
        case 1:
            _formatOutput("Prefix", ctx->message_tx_obj->prefix.len, ctx->message_tx_obj->prefix.ptr, outKey, outKeyLen,
                          outVal, outValLen, pageIdx, pageCount);
            break;
        case 2:
            snprintf(outKey, outKeyLen, "Domain");
            snprintf(outVal, outValLen, "%s", _domainToString(ctx->message_tx_obj->domain));
            break;
        case 3:
            _formatOutput("Msg", ctx->message_tx_obj->message.len, ctx->message_tx_obj->message.ptr, outKey, outKeyLen,
                          outVal, outValLen, pageIdx, pageCount);
            break;
        default:
            return parser_no_data;
    }

    return parser_ok;
}
