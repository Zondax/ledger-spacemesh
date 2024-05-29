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

// #{TODO} --> Apply tests that check this app's encoding/libraries

#include "gmock/gmock.h"

#include <vector>
#include <iostream>
#include <hexutils.h>
#include "parser_txdef.h"
#include "parser.h"
#include "parser_impl.h"

using namespace std;

TEST(SCALE, ReadBytes) {
    parser_context_t ctx;
    parser_tx_t tx_obj;
    parser_error_t err;
    uint8_t buffer[100];
    auto bufferLen = parseHexString(
            buffer,
            sizeof(buffer),
            "45"
            "123456"
            "12345678901234567890"
    );

    parser_parse(&ctx, buffer, bufferLen, &tx_obj);

    // uint8_t bytesArray[100] = {0};
    // err = _readBytes(&ctx, bytesArray, 1);
    // EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    // EXPECT_EQ(bytesArray[0], 0x45);

    // uint8_t testArray[3] = {0x12, 0x34, 0x56};
    // err = _readBytes(&ctx, bytesArray+1, 3);
    // EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    // for (uint8_t i = 0; i < 3; i++) {
    //     EXPECT_EQ(testArray[i], bytesArray[i+1]);
    // }

    // uint8_t testArray2[10] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90};
    // err = _readBytes(&ctx, bytesArray+4, 10);
    // EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    // for (uint8_t i = 0; i < 10; i++) {
    //     EXPECT_EQ(testArray2[i], bytesArray[i+4]);
    // }
}
