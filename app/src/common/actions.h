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

#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "apdu_codes.h"
#include "coin.h"
#include "crypto.h"
#include "crypto_helper.h"
#include "parser_txdef.h"
#include "tx.h"
#include "zxerror.h"
#include "zxformat.h"

extern uint16_t action_addrResponseLen;
extern address_request_t addr_request;

__Z_INLINE zxerr_t app_fill_address() {
    // Put data directly in the apdu buffer
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);

    action_addrResponseLen = 0;
    const zxerr_t err = crypto_fillAddress(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE, &action_addrResponseLen);

    if (err != zxerr_ok || action_addrResponseLen == 0) {
        THROW(APDU_CODE_EXECUTION_ERROR);
    }

    return zxerr_ok;
}

__Z_INLINE zxerr_t app_fill_address_multisig() {
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    action_addrResponseLen = 0;

    CHECK_ZXERR(crypto_fillAddressMultisigOrVesting(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE, &action_addrResponseLen));

    addr_request.account_type = MULTISIG;
    return zxerr_ok;
}

__Z_INLINE zxerr_t app_fill_address_vesting() {
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    action_addrResponseLen = 0;

    CHECK_ZXERR(crypto_fillAddressMultisigOrVesting(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE, &action_addrResponseLen));

    addr_request.account_type = VESTING;
    return zxerr_ok;
}

__Z_INLINE zxerr_t app_fill_address_vault() {
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    action_addrResponseLen = 0;

    CHECK_ZXERR(crypto_fillAddressVault(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE, &action_addrResponseLen));

    addr_request.account_type = VAULT;
    return zxerr_ok;
}

__Z_INLINE void app_sign() {
    const uint8_t *message = tx_get_buffer();
    const uint16_t messageLength = tx_get_buffer_length();

    const zxerr_t err = crypto_sign(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE - 3, message, messageLength);

    if (err != zxerr_ok) {
        set_code(G_io_apdu_buffer, 0, APDU_CODE_SIGN_VERIFY_ERROR);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    } else {
        set_code(G_io_apdu_buffer, SK_LEN_25519, APDU_CODE_OK);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, SK_LEN_25519 + 2);
    }
}

__Z_INLINE void app_message_sign() {
    // Skip the first 4 bytes for prefix and message length
    // payload = [prefixLen(2 bytes), messageLen(2 bytes), prefix, domain, message]
    const uint8_t *message = tx_get_buffer() + PARSER_MESSAGE_PREFIX_LEN + PARSER_MESSAGE_MESSAGE_LEN;
    const uint16_t messageLength = tx_get_buffer_length() - PARSER_MESSAGE_PREFIX_LEN - PARSER_MESSAGE_MESSAGE_LEN;

    const zxerr_t err = crypto_sign(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE - 3, message, messageLength);

    if (err != zxerr_ok) {
        set_code(G_io_apdu_buffer, 0, APDU_CODE_SIGN_VERIFY_ERROR);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    } else {
        set_code(G_io_apdu_buffer, SK_LEN_25519, APDU_CODE_OK);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, SK_LEN_25519 + 2);
    }
}

__Z_INLINE void app_reject() {
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

__Z_INLINE void app_reply_address() {
    set_code(G_io_apdu_buffer, action_addrResponseLen, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, action_addrResponseLen + 2);
}

__Z_INLINE void app_reply_error() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_DATA_INVALID);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}
