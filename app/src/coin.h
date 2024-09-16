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

#define CLA 0x45

#define HDPATH_LEN_DEFAULT 5
#define HDPATH_0_DEFAULT (0x80000000u | 0x2c)   // 44
#define HDPATH_1_DEFAULT (0x80000000u | 0x21c)  // 540
#define HDPATH_1_TESTNET (0x80000000u | 0x01)   // 1
#define GENESIS_BYTES_MAINNET "9eebff023abb17ccb775c602daade8ed708f0a50"

#define HDPATH_2_DEFAULT (0x80000000u | 0u)
#define HDPATH_3_DEFAULT (0u)
#define HDPATH_4_DEFAULT (0u)

#define SECP256K1_PK_LEN 65u

#define SK_LEN_25519 64u
#define SCALAR_LEN_ED25519 32u
#define SIG_PLUS_TYPE_LEN 65u

#define ED25519_SIGNATURE_SIZE 64u

#define PUB_KEY_LENGTH 32u
#define SS58_ADDRESS_MAX_LEN 60u
#define GENESIS_ID_LENGTH 20

#define MAX_SIGN_SIZE 256u
#define BLAKE2B_DIGEST_SIZE 32u

// Instructions
#define INS_GET_ADDR_MULTISIG 0x03
#define INS_GET_ADDR_VESTING 0x04
#define INS_GET_ADDR_VAULT 0x05
#define INS_SIGN_MESSAGE 0x06

#define COIN_AMOUNT_DECIMAL_PLACES 9
#define COIN_TICKER "SMH "
#define COIN_BASIC_UNIT "SMIDGE "

#define MENU_MAIN_APP_LINE1 "Spacemesh"
#define MENU_MAIN_APP_LINE2 "Ready"
#define APPVERSION_LINE1 "Spacemesh"
#define APPVERSION_LINE2 "v" APPVERSION

#ifdef __cplusplus
}
#endif
