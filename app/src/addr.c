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

#include "addr.h"

#include <stdio.h>

#include "app_mode.h"
#include "coin.h"
#include "crypto.h"
#include "crypto_helper.h"
#include "os.h"
#include "parser_txdef.h"
#include "tx.h"
#include "zxerror.h"
#include "zxformat.h"
#include "zxmacros.h"

address_request_t addr_request = {0};

#define BUFF_FIRST_ACCOUNT_INDEX 3
#define VAULT_INFO_SIZE 24  // TODO: avoid hardcoding sizes

zxerr_t clearAddressRequest() {
    addr_request.account_type = UNKNOWN;
    addr_request.optional_numberOfPubkeys = 0;
    return zxerr_ok;
}

zxerr_t readAddressRequest(account_type_e account_type) {
    if (addr_request.account_type != UNKNOWN || account_type == UNKNOWN) {
        return zxerr_unknown;
    }

    const uint8_t *message = tx_get_buffer();
    const uint16_t messageLength = tx_get_buffer_length();

    switch (account_type) {
        case MULTISIG:
        case VESTING: {
            addr_request.internalIndex = message[0];

            // TODO: misaligned access throw in this CPU. Be careful with misaligned u32
            addr_request.account = (generic_account_t *)(message + 1);

            uint8_t pubkeysBuffSize = messageLength - BUFF_FIRST_ACCOUNT_INDEX;
            // Ensure the buffer size is a multiple of pubkey_item_t
            if (pubkeysBuffSize % sizeof(pubkey_item_t) != 0) {
                return zxerr_invalid_crypto_settings;
            }

            addr_request.optional_numberOfPubkeys = (pubkeysBuffSize / sizeof(pubkey_item_t));

            // Validate the number of public keys
            if (addr_request.optional_numberOfPubkeys + 1 != addr_request.account->participants) {
                return zxerr_invalid_crypto_settings;
            }

            // validate account
            uint8_t indexAux = 0;
            for (int i = 0; i < addr_request.optional_numberOfPubkeys + 1; i++) {
                if (i == addr_request.internalIndex) {
                    continue;
                }
                if (i == addr_request.account->keys[indexAux].index) {
                    indexAux++;
                } else {
                    return zxerr_invalid_crypto_settings;
                }
            }
            break;
        }
        case VAULT: {
            addr_request.internalIndex = message[0];
            addr_request.vault_account = (vault_account_t *)(message + 1);

            uint8_t pubkeysBuffSize = messageLength - (BUFF_FIRST_ACCOUNT_INDEX + VAULT_INFO_SIZE);
            // Ensure the buffer size is a multiple of pubkey_item_t s
            if (pubkeysBuffSize % sizeof(pubkey_item_t) != 0) {
                return zxerr_invalid_crypto_settings;
            }

            addr_request.optional_numberOfPubkeys = (pubkeysBuffSize / sizeof(pubkey_item_t));

            // Validate the number of public keys
            if (addr_request.optional_numberOfPubkeys + 1 != addr_request.vault_account->owner.participants) {
                return zxerr_invalid_crypto_settings;
            }

            // create account
            uint8_t indexAux = 0;
            for (int i = 0; i < addr_request.optional_numberOfPubkeys + 1; i++) {
                if (i == addr_request.internalIndex) {
                    continue;
                }
                if (i == addr_request.vault_account->owner.keys[indexAux].index) {
                    indexAux++;
                } else {
                    return zxerr_invalid_crypto_settings;
                }
            }
            break;
        }
        default:
            return zxerr_encoding_failed;
    }

    // Only where everything is fine, mark the account type
    addr_request.account_type = account_type;
    return zxerr_ok;
}

zxerr_t wallet_getNumItems(uint8_t *num_items) {
    zemu_log_stack("wallet_getNumItems");
    *num_items = 1;
    if (app_mode_expert()) {
        *num_items = 2;
    }
    return zxerr_ok;
}

zxerr_t wallet_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen,
                       uint8_t pageIdx, uint8_t *pageCount) {
    ZEMU_LOGF(50, "[wallet_getItem] %d/%d\n", displayIdx, pageIdx)
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Address");
            pageString(outVal, outValLen, (char *)(G_io_apdu_buffer + PUB_KEY_LENGTH), pageIdx, pageCount);
            return zxerr_ok;
        case 1: {
            if (!app_mode_expert()) {
                return zxerr_no_data;
            }

            snprintf(outKey, outKeyLen, "Your Path");
            char buffer[300];
            bip32_to_str(buffer, sizeof(buffer), hdPath, HDPATH_LEN_DEFAULT);
            pageString(outVal, outValLen, buffer, pageIdx, pageCount);
            return zxerr_ok;
        }
        default:
            return zxerr_no_data;
    }
}

static zxerr_t getPublicKey(const uint8_t index, const uint8_t internalIndex, pubkey_item_t *keys,
                            const uint8_t **pubkeyPtr) {
    if (keys == NULL || pubkeyPtr == NULL) {
        return zxerr_no_data;
    }
    if (index == internalIndex) {
        *pubkeyPtr = G_io_apdu_buffer;
        return zxerr_ok;
    }

    // On the tx_buffer we have only external pubkeys
    const uint8_t tmpIndex = index > internalIndex ? (index - 1) : index;
    *pubkeyPtr = keys[tmpIndex].pubkey;
    return zxerr_ok;
}

zxerr_t multisigVesting_getNumItems(uint8_t *num_items) {
    ZEMU_LOGF(50, "multisigVesting_getNumItems\n");

    if (addr_request.account_type != MULTISIG && addr_request.account_type != VESTING) {
        return zxerr_encoding_failed;
    }

    const uint8_t fixedFields = 5;  // Participants, Approvers, internal pubkey
                                    // + address + path

    // Everything from above
    *num_items = addr_request.optional_numberOfPubkeys + fixedFields;
    return zxerr_ok;
}

zxerr_t multisigVesting_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen,
                                uint8_t pageIdx, uint8_t *pageCount) {
    ZEMU_LOGF(50, "[multisigVesting_getItem] %d/%d\n", displayIdx, pageIdx)

    if (addr_request.account_type != MULTISIG && addr_request.account_type != VESTING) {
        return zxerr_invalid_crypto_settings;
    }

    uint8_t numItems = 0;
    multisigVesting_getNumItems(&numItems);

    if (displayIdx >= numItems) {
        return zxerr_no_data;
    }

    *pageCount = 1;
    switch ((uint8_t)displayIdx) {
        case 0:
            if (addr_request.account_type == MULTISIG) {
                snprintf(outKey, outKeyLen, "Multisig");
            } else {
                snprintf(outKey, outKeyLen, "Vesting");
            }
            pageString(outVal, outValLen, (char *)(G_io_apdu_buffer + PUB_KEY_LENGTH), pageIdx, pageCount);
            return zxerr_ok;

        case 1: {
            snprintf(outKey, outKeyLen, "HD Path");
            char buffer[300] = {0};
            bip32_to_str(buffer, sizeof(buffer), hdPath, HDPATH_LEN_DEFAULT);
            pageString(outVal, outValLen, buffer, pageIdx, pageCount);
            return zxerr_ok;
        }

        case 2:
            snprintf(outKey, outKeyLen, "Participants");
            snprintf(outVal, outValLen, "%d", addr_request.account->participants);
            return zxerr_ok;

        case 3:
            snprintf(outKey, outKeyLen, "Validators");
            snprintf(outVal, outValLen, "%d", addr_request.account->approvers);
            return zxerr_ok;

        case 255:
            if (addr_request.account_type == MULTISIG) {
                snprintf(outVal, outKeyLen, "Review Multisig address");
            } else {
                snprintf(outVal, outKeyLen, "Review Vesting address");
            }
            return zxerr_ok;

        default: {
            const uint8_t tmpDisplayIdx = displayIdx - 4;
            const uint8_t *pubkeyPtr = NULL;
            snprintf(outKey, outKeyLen, "Pubkey %d", tmpDisplayIdx);
            CHECK_ZXERR(getPublicKey(tmpDisplayIdx, addr_request.internalIndex, addr_request.account->keys, &pubkeyPtr))
            pageStringHex(outVal, outValLen, (const char *)pubkeyPtr, PUB_KEY_LENGTH, pageIdx, pageCount);
            return zxerr_ok;
        }
    }

    return zxerr_no_data;
}

zxerr_t vault_getNumItems(uint8_t *num_items) {
    ZEMU_LOGF(50, "vault_getNumItems\n");

    if (addr_request.account_type != VAULT) {
        return zxerr_encoding_failed;
    }

    // TotalAmount, InitialUnlockAmount, vestingStart, vestingEnd, Participants, Approvers, internal pubkey
    const uint8_t fixedFields = 7;

    // Everything from above + address + path
    *num_items = addr_request.optional_numberOfPubkeys + fixedFields + 2;
    return zxerr_ok;
}

zxerr_t vault_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen, uint8_t pageIdx,
                      uint8_t *pageCount) {
    ZEMU_LOGF(50, "vault_getItem %d/%d\n", displayIdx, pageIdx)

    if (addr_request.account_type != VAULT) {
        return zxerr_encoding_failed;
    }

    uint8_t numItems = 0;
    vault_getNumItems(&numItems);
    if (displayIdx >= numItems) {
        return zxerr_no_data;
    }
    *pageCount = 1;

    char tmpBuffer[30] = {0};
    switch ((uint8_t)displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Vault");
            snprintf(outVal, outValLen, "%d", addr_request.internalIndex);
            pageString(outVal, outValLen, (char *)(G_io_apdu_buffer + PUB_KEY_LENGTH), pageIdx, pageCount);
            break;

        case 1: {
            snprintf(outKey, outKeyLen, "HD Path");
            char buffer[300] = {0};
            bip32_to_str(buffer, sizeof(buffer), hdPath, HDPATH_LEN_DEFAULT);
            pageString(outVal, outValLen, buffer, pageIdx, pageCount);
            break;
        }

        case 2: {
            snprintf(outKey, outKeyLen, "TotalAmount");
            if (uint64_to_str(tmpBuffer, sizeof(tmpBuffer), addr_request.vault_account->totalAmount) != NULL) {
                return zxerr_unknown;
            }
            pageStringExt(outVal, outValLen, tmpBuffer, sizeof(tmpBuffer), pageIdx, pageCount);
            break;
        }

        case 3: {
            snprintf(outKey, outKeyLen, "InitialUnlock");
            if (uint64_to_str(tmpBuffer, sizeof(tmpBuffer), addr_request.vault_account->initialUnlockAmount) != NULL) {
                return zxerr_unknown;
            }
            pageStringExt(outVal, outValLen, tmpBuffer, sizeof(tmpBuffer), pageIdx, pageCount);
            break;
        }

        case 4:
            snprintf(outKey, outKeyLen, "Start");
            snprintf(outVal, outValLen, "%d", addr_request.vault_account->vestingStart);
            break;

        case 5:
            snprintf(outKey, outKeyLen, "End");
            snprintf(outVal, outValLen, "%d", addr_request.vault_account->vestingEnd);
            break;

        case 6:
            snprintf(outKey, outKeyLen, "Participants");
            snprintf(outVal, outValLen, "%d", addr_request.vault_account->owner.participants);
            break;

        case 7:
            snprintf(outKey, outKeyLen, "Validators");
            snprintf(outVal, outValLen, "%d", addr_request.vault_account->owner.approvers);
            break;

        case 255:
            snprintf(outVal, outKeyLen, "Review Vault address");
            break;

        default: {
            const uint8_t tmpDisplayIdx = displayIdx - 8;
            const uint8_t *pubkeyPtr = NULL;
            snprintf(outKey, outKeyLen, "Pubkey %d", tmpDisplayIdx);
            CHECK_ZXERR(
                getPublicKey(tmpDisplayIdx, addr_request.internalIndex, addr_request.vault_account->owner.keys, &pubkeyPtr))
            pageStringHex(outVal, outValLen, (const char *)pubkeyPtr, PUB_KEY_LENGTH, pageIdx, pageCount);
            break;
        }
    }

    return zxerr_ok;
}
