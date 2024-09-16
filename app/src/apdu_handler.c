/*******************************************************************************
 *   (c) 2018 - 2024 Zondax AG
 *   (c) 2016 Ledger
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

#include <os.h>
#include <os_io_seproxyhal.h>
#include <string.h>
#include <ux.h>

#include "actions.h"
#include "addr.h"
#include "app_main.h"
#include "coin.h"
#include "crypto.h"
#include "crypto_helper.h"
#include "parser.h"
#include "tx.h"
#include "view.h"
#include "view_internal.h"
#include "zxmacros.h"

static bool tx_initialized = false;
static bool genesisId_extracted = false;
static bool requireConfirmation = false;
extern account_type_e addr_review_account_type;

static const char *MULTISIG_TITLE = "Multisig address";
static const char *VESTING_TITLE = "Vesting address";
static const char *VAULT_TITLE = "Vault address";
static const char *VALIDATE_LABEL = "Tap to validate";

#define CATCH_ZXERR_WITH_MESSAGE(zxerr)                                            \
    if (zxerr != zxerr_ok) {                                                       \
        const char *error_msg = parser_getZxErrorDescription(zxerr);               \
        const int error_msg_length = strnlen(error_msg, sizeof(G_io_apdu_buffer)); \
        memcpy(G_io_apdu_buffer, error_msg, error_msg_length);                     \
        *tx = error_msg_length;                                                    \
        THROW(APDU_CODE_DATA_INVALID);                                             \
    }

void extractHDPath(uint32_t rx, uint32_t offset) {
    tx_initialized = false;

    if ((rx - offset) != sizeof(uint32_t) * HDPATH_LEN_DEFAULT) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    memcpy(hdPath, G_io_apdu_buffer + offset, sizeof(uint32_t) * HDPATH_LEN_DEFAULT);

    const bool mainnet = hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT;
    const bool testnet = hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_TESTNET;

    if (!mainnet && !testnet) {
        THROW(APDU_CODE_DATA_INVALID);
    }
}

void extractGenesisId(uint32_t rx, uint32_t offset) {
    if ((rx - offset) < sizeof(genesisId)) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    memcpy(genesisId, G_io_apdu_buffer + offset, sizeof(genesisId));

    bool hdPath_mainnet = hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT;

    uint8_t genesis_bytes_mainnet[GENESIS_ID_LENGTH];
    hexstr_to_array(genesis_bytes_mainnet, GENESIS_ID_LENGTH, GENESIS_BYTES_MAINNET, strlen(GENESIS_BYTES_MAINNET));
    bool genesis_mainnet = memcmp(genesisId, genesis_bytes_mainnet, GENESIS_ID_LENGTH) == 0;

    if (!hdPath_mainnet && genesis_mainnet) {
        THROW(APDU_CODE_DATA_INVALID);
    }
}

__Z_INLINE bool process_chunk(__Z_UNUSED volatile uint32_t *tx, uint32_t rx, bool genesisIdAvailable) {
    const uint8_t payloadType = G_io_apdu_buffer[OFFSET_PAYLOAD_TYPE];
    if (rx < OFFSET_DATA) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    uint32_t added;
    bool handleGenesisId = !genesisId_extracted && genesisIdAvailable;
    switch (payloadType) {
        case P1_INIT:
            tx_initialize();
            tx_reset();
            extractHDPath(rx, OFFSET_DATA);
            requireConfirmation = G_io_apdu_buffer[OFFSET_P1];
            tx_initialized = true;
            genesisId_extracted = false;
            return false;
        case P1_ADD:
            if (handleGenesisId) {
                extractGenesisId(rx, OFFSET_DATA);
                genesisId_extracted = true;
            }
            if (!tx_initialized) {
                THROW(APDU_CODE_TX_NOT_INITIALIZED);
            }
            added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);
            if (added != rx - OFFSET_DATA) {
                tx_initialized = false;
                THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
            }
            return false;
        case P1_LAST:
            if (handleGenesisId) {
                extractGenesisId(rx, OFFSET_DATA);
                genesisId_extracted = true;
            }
            if (!tx_initialized) {
                THROW(APDU_CODE_TX_NOT_INITIALIZED);
            }
            added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);
            tx_initialized = false;
            if (added != rx - OFFSET_DATA) {
                tx_initialized = false;
                THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
            }
            tx_initialized = false;
            return true;
    }

    THROW(APDU_CODE_INVALIDP1P2);
}

// Handle Wallet addresses
__Z_INLINE void handleGetAddr(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    ZEMU_LOGF(50, "handleGetAddr %d\n", rx);
    if (!process_chunk(tx, rx, true)) {
        THROW(APDU_CODE_OK);
    }

    requireConfirmation = G_io_apdu_buffer[OFFSET_P2];

    CATCH_ZXERR_WITH_MESSAGE(app_fill_address());

    if (requireConfirmation) {
        view_review_init(wallet_getItem, wallet_getNumItems, app_reply_address);
        view_review_show(REVIEW_ADDRESS);
        *flags |= IO_ASYNCH_REPLY;
        return;
    }
    *tx = action_addrResponseLen;
    THROW(APDU_CODE_OK);
}

__Z_INLINE void handleSign(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    ZEMU_LOGF(50, "handleSign %d\n", rx);
    if (!process_chunk(tx, rx, true)) {
        THROW(APDU_CODE_OK);
    }

    *tx = 0;
    const char *error_msg = tx_parse();
    CHECK_APP_CANARY()
    if (error_msg != NULL) {
        const int error_msg_length = strnlen(error_msg, sizeof(G_io_apdu_buffer));
        memcpy(G_io_apdu_buffer, error_msg, error_msg_length);
        *tx += (error_msg_length);
        THROW(APDU_CODE_DATA_INVALID);
    }

    view_review_init(tx_getItem, tx_getNumItems, app_sign);
    view_review_show(REVIEW_TXN);
    *flags |= IO_ASYNCH_REPLY;
}

__Z_INLINE void handleSignMessage(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    ZEMU_LOGF(50, "handleSignMessage\n");
    if (!process_chunk(tx, rx, false)) {
        THROW(APDU_CODE_OK);
    }

    *tx = 0;
    const char *error_msg = tx_message_parse();
    CHECK_APP_CANARY()
    if (error_msg != NULL) {
        const int error_msg_length = strnlen(error_msg, sizeof(G_io_apdu_buffer));
        memcpy(G_io_apdu_buffer, error_msg, error_msg_length);
        *tx += (error_msg_length);
        THROW(APDU_CODE_DATA_INVALID);
    }

    view_review_init(tx_message_getItem, tx_message_getNumItems, app_message_sign);
    view_review_show(REVIEW_TXN);
    *flags |= IO_ASYNCH_REPLY;
}

// Handle Multisig, Vesting and Vault addresses
__Z_INLINE void handleMultisig(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx, account_type_e account_type) {
    ZEMU_LOGF(50, "handleMultisig %d\n", rx);
    if (!process_chunk(tx, rx, true)) {
        THROW(APDU_CODE_OK);
    }

    clearAddressRequest();
    CATCH_ZXERR_WITH_MESSAGE(readAddressRequest(account_type));

    const char *title = NULL;
    switch (account_type) {
        case MULTISIG:
            title = PIC(MULTISIG_TITLE);
            CATCH_ZXERR_WITH_MESSAGE(app_fill_address_multisig());
            view_review_init(multisigVesting_getItem, multisigVesting_getNumItems, app_reply_address);
            break;
        case VESTING:
            title = PIC(VESTING_TITLE);
            CATCH_ZXERR_WITH_MESSAGE(app_fill_address_vesting());
            view_review_init(multisigVesting_getItem, multisigVesting_getNumItems, app_reply_address);
            break;
        case VAULT:
            title = PIC(VAULT_TITLE);
            CATCH_ZXERR_WITH_MESSAGE(app_fill_address_vault());
            view_review_init(vault_getItem, vault_getNumItems, app_reply_address);
            break;
        default:
            THROW(APDU_CODE_DATA_INVALID);
            break;
    }

    view_review_show_generic(REVIEW_GENERIC, title, PIC(VALIDATE_LABEL));
    *flags |= IO_ASYNCH_REPLY;
}

__Z_INLINE void handle_getversion(__Z_UNUSED volatile uint32_t *flags, volatile uint32_t *tx) {
    G_io_apdu_buffer[0] = 0;

#if defined(APP_TESTING)
    G_io_apdu_buffer[0] = 0x01;
#endif

    G_io_apdu_buffer[1] = (LEDGER_MAJOR_VERSION >> 8) & 0xFF;
    G_io_apdu_buffer[2] = (LEDGER_MAJOR_VERSION >> 0) & 0xFF;

    G_io_apdu_buffer[3] = (LEDGER_MINOR_VERSION >> 8) & 0xFF;
    G_io_apdu_buffer[4] = (LEDGER_MINOR_VERSION >> 0) & 0xFF;

    G_io_apdu_buffer[5] = (LEDGER_PATCH_VERSION >> 8) & 0xFF;
    G_io_apdu_buffer[6] = (LEDGER_PATCH_VERSION >> 0) & 0xFF;

    G_io_apdu_buffer[7] = 0;

    G_io_apdu_buffer[8] = (TARGET_ID >> 24) & 0xFF;
    G_io_apdu_buffer[9] = (TARGET_ID >> 16) & 0xFF;
    G_io_apdu_buffer[10] = (TARGET_ID >> 8) & 0xFF;
    G_io_apdu_buffer[11] = (TARGET_ID >> 0) & 0xFF;

    *tx += 12;
    THROW(APDU_CODE_OK);
}

#if defined(APP_TESTING)
void handleTest(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    (void)flags;
    (void)tx;
    (void)rx;
    THROW(APDU_CODE_OK);
}
#endif

void handleApdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    volatile uint16_t sw = 0;

    BEGIN_TRY {
        TRY {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(APDU_CODE_CLA_NOT_SUPPORTED);
            }

            if (rx < APDU_MIN_LENGTH) {
                THROW(APDU_CODE_WRONG_LENGTH);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case INS_GET_VERSION: {
                    handle_getversion(flags, tx);
                    break;
                }

                case INS_GET_ADDR: {
                    CHECK_PIN_VALIDATED()
                    handleGetAddr(flags, tx, rx);
                    break;
                }

                case INS_SIGN: {
                    CHECK_PIN_VALIDATED()
                    handleSign(flags, tx, rx);
                    break;
                }

                case INS_GET_ADDR_MULTISIG: {
                    CHECK_PIN_VALIDATED()
                    handleMultisig(flags, tx, rx, MULTISIG);
                    break;
                }

                case INS_GET_ADDR_VESTING: {
                    CHECK_PIN_VALIDATED()
                    handleMultisig(flags, tx, rx, VESTING);
                    break;
                }

                case INS_GET_ADDR_VAULT: {
                    CHECK_PIN_VALIDATED()
                    handleMultisig(flags, tx, rx, VAULT);
                    break;
                }

                case INS_SIGN_MESSAGE: {
                    CHECK_PIN_VALIDATED()
                    handleSignMessage(flags, tx, rx);
                    break;
                }

#if defined(APP_TESTING)
                case INS_TEST: {
                    handleTest(flags, tx, rx);
                    THROW(APDU_CODE_OK);
                    break;
                }
#endif
                default:
                    THROW(APDU_CODE_INS_NOT_SUPPORTED);
            }
        }
        CATCH(EXCEPTION_IO_RESET) { THROW(EXCEPTION_IO_RESET); }
        CATCH_OTHER(e) {
            switch (e & 0xF000) {
                case 0x6000:
                case APDU_CODE_OK:
                    sw = e;
                    break;
                default:
                    sw = 0x6800 | (e & 0x7FF);
                    break;
            }
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw & 0xFF;
            *tx += 2;
        }
        FINALLY {}
    }
    END_TRY;
}
