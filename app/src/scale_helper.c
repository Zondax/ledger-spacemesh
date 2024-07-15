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
#include "parser_common.h"
#include "parser_impl.h"

parser_error_t readBytes(parser_context_t *ctx, Bytes_t *val) {
    CHECK_INPUT();

    CompactInt_t clen = {0};
    CHECK_ERROR(readCompactInt(ctx, &clen));
    uint64_t len;
    CHECK_ERROR(_getValue(&clen, &len));
    val->len = (uint16_t)len;

    val->ptr = ctx->buffer + ctx->offset;
    CTX_CHECK_AND_ADVANCE(ctx, val->len);

    return parser_ok;
}

parser_error_t readFixedArray(parser_context_t *ctx, Bytes_t *val, uint16_t size) {
    CHECK_INPUT();

    val->ptr = ctx->buffer + ctx->offset;
    val->len = size;
    CTX_CHECK_AND_ADVANCE(ctx, size);

    return parser_ok;
}

parser_error_t readCompactU64(parser_context_t *ctx, uint64_t *val) {
    CHECK_INPUT();

    CompactInt_t tmp = {0};
    CHECK_ERROR(readCompactInt(ctx, &tmp));
    CHECK_ERROR(_getValue(&tmp, val));

    return parser_ok;
}

parser_error_t readCompactU32(parser_context_t *ctx, uint32_t *val) {
    CHECK_INPUT();

    CompactInt_t tmp = {0};
    uint64_t tmpValue = 0;
    CHECK_ERROR(readCompactInt(ctx, &tmp));
    CHECK_ERROR(_getValue(&tmp, &tmpValue));

    if (tmpValue > UINT32_MAX) {
        return parser_value_out_of_range;
    }

    *val = (uint32_t)tmpValue;

    return parser_ok;
}

parser_error_t readCompactU8(parser_context_t *ctx, uint8_t *val) {
    CHECK_INPUT();

    CompactInt_t tmp = {0};
    uint64_t tmpValue = 0;
    CHECK_ERROR(readCompactInt(ctx, &tmp));
    CHECK_ERROR(_getValue(&tmp, &tmpValue));

    if (tmpValue > UINT8_MAX) {
        return parser_value_out_of_range;
    }

    *val = (uint8_t)tmpValue;

    return parser_ok;
}
