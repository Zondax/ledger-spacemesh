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

#include <zxmacros.h>

#include "parser_common.h"
#include "parser_txdef.h"
#include "zxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Checks that there are at least SIZE bytes available in the buffer
#define CTX_CHECK_AVAIL(CTX, SIZE)                                      \
    if ((CTX) == NULL || ((CTX)->offset + (SIZE)) > (CTX)->bufferLen) { \
        return parser_unexpected_buffer_end;                            \
    }

#define CTX_CHECK_AND_ADVANCE(CTX, SIZE) \
    CTX_CHECK_AVAIL((CTX), (SIZE))       \
    (CTX)->offset += (SIZE);

// Checks function input is valid
#define CHECK_INPUT()          \
    if (val == NULL) {         \
        return parser_no_data; \
    }                          \
    CTX_CHECK_AVAIL(ctx, 1)  // Checks that there is something available in the buffer

parser_error_t _read(parser_context_t *c, parser_tx_t *v);

/**
 * @brief Reads a compact integer from the context.
 * @param ctx Parser context
 * @param val Pointer to store the compact integer
 * @return parser_error_t Error code
 */
parser_error_t readCompactInt(parser_context_t *ctx, CompactInt_t *val);

/**
 * @brief Gets the value from a compact integer.
 * @param compact Pointer to the compact integer
 * @param value Pointer to store the value
 * @return parser_error_t Error code
 */
parser_error_t _getValue(const CompactInt_t *compact, uint64_t *value);

parser_error_t _readTxVersion(parser_context_t *ctx, uint8_t *val);
parser_error_t _readMethodSelector(parser_context_t *ctx, uint8_t *val);
parser_error_t _readSpawnTx(parser_context_t *ctx, parser_tx_t *val);
parser_error_t _readSpendTx(parser_context_t *ctx, parser_tx_t *val);

#ifdef __cplusplus
}
#endif
