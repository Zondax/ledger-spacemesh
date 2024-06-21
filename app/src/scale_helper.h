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

#include <stdint.h>

#include "parser_common.h"

#ifdef __cplusplus
extern "C" {
#endif

parser_error_t readBytes(parser_context_t *ctx, Bytes_t *val);
parser_error_t readFixedArray(parser_context_t *ctx, Bytes_t *val, uint16_t size);

parser_error_t readCompactU64(parser_context_t *ctx, uint64_t *val);
parser_error_t readCompactU32(parser_context_t *ctx, uint32_t *val);
parser_error_t readCompactU8(parser_context_t *ctx, uint8_t *val);

#ifdef __cplusplus
}
#endif