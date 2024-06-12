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

extern account_type_e accountType;

zxerr_t wallet_getNumItems(uint8_t *num_items) {
    zemu_log_stack("addr_getNumItems");
    *num_items = 1;
    if (app_mode_expert()) {
        *num_items = 2;
    }
    return zxerr_ok;
}

zxerr_t wallet_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen,
                       uint8_t pageIdx, uint8_t *pageCount) {
    ZEMU_LOGF(50, "[addr_getItem] %d/%d\n", displayIdx, pageIdx)
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

static zxerr_t getPublicKey(const uint8_t index, const uint8_t internalIndex, pubkey_t *keys,
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

    const uint8_t fixedFields = 3;  // Participants, Approvers, internal pubkey
    const uint8_t totalExternalPubkeys = (tx_get_buffer_length() - fixedFields) / 33;

    // Everything from above + address + path
    *num_items = totalExternalPubkeys + fixedFields + 2;
    return zxerr_ok;
}

zxerr_t multisigVesting_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen,
                                uint8_t pageIdx, uint8_t *pageCount) {
    ZEMU_LOGF(50, "[addr_getItem] %d/%d\n", displayIdx, pageIdx)
    uint8_t numItems = 0;
    multisigVesting_getNumItems(&numItems);

    if (displayIdx >= numItems) {
        return zxerr_no_data;
    }

    if (accountType != MULTISIG && accountType != VESTING) {
        return zxerr_invalid_crypto_settings;
    }

    // [internalIndex | approvers | participants [idx|pubkey]]
    account_t *account = (account_t *)tx_get_buffer();

    *pageCount = 1;
    switch ((uint8_t)displayIdx) {
        case 0:
            if (accountType == MULTISIG) {
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
            snprintf(outVal, outValLen, "%d", account->participants);
            return zxerr_ok;

        case 3:
            snprintf(outKey, outKeyLen, "Validators");
            snprintf(outVal, outValLen, "%d", account->approvers);
            return zxerr_ok;

        case 255:
            if (accountType == MULTISIG) {
                snprintf(outVal, outKeyLen, "Review Multisig address");
            } else {
                snprintf(outVal, outKeyLen, "Review Vesting address");
            }
            return zxerr_ok;

        default: {
            const uint8_t tmpDisplayIdx = displayIdx - 4;
            const uint8_t *pubkeyPtr = NULL;
            snprintf(outKey, outKeyLen, "Pubkey %d", tmpDisplayIdx);
            CHECK_ZXERR(getPublicKey(tmpDisplayIdx, account->internalIndex, account->keys, &pubkeyPtr))
            pageStringHex(outVal, outValLen, (const char *)pubkeyPtr, PUB_KEY_LENGTH, pageIdx, pageCount);
            return zxerr_ok;
        }
    }

    return zxerr_no_data;
}

zxerr_t vault_getNumItems(uint8_t *num_items) {
    ZEMU_LOGF(50, "vault_getNumItems\n");

// TotalAmount, InitialUnlockAmount, vestingStart, vestingEnd, Participants, Approvers, internal pubkey
    const uint8_t fixedFields = 7;
    const uint8_t totalExternalPubkeys = (tx_get_buffer_length() - fixedFields) / 33;

    // Everything from above + address + path
    *num_items = totalExternalPubkeys + fixedFields + 2;
    return zxerr_ok;
}

zxerr_t vault_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen, uint8_t pageIdx,
                      uint8_t *pageCount) {
    ZEMU_LOGF(50, "vault_getItem %d/%d\n", displayIdx, pageIdx)
    uint8_t numItems = 0;
    vault_getNumItems(&numItems);
    if (displayIdx >= numItems) {
        return zxerr_no_data;
    }

    // [totalAmount | initialUnlockAmount | vestingStart | vestingEnd
    //  internalIndex | approvers | participants [idx|pubkey] ]
    vault_account_t *vault = (vault_account_t *)tx_get_buffer();
    char tmpBuffer[30] = {0};
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Vault");
            snprintf(outVal, outValLen, "%d", vault->owner.internalIndex);
            // pageString(outVal, outValLen, (char *)(G_io_apdu_buffer + PUB_KEY_LENGTH), pageIdx, pageCount);
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
            if (uint64_to_str(tmpBuffer, sizeof(tmpBuffer), vault->totalAmount) != NULL) {
                return zxerr_unknown;
            }
            pageStringExt(outVal, outValLen, tmpBuffer, sizeof(tmpBuffer), pageIdx, pageCount);
            break;
        }

        case 3: {
            snprintf(outKey, outKeyLen, "InitialUnlock");
            if (uint64_to_str(tmpBuffer, sizeof(tmpBuffer), vault->initialUnlockAmount) != NULL) {
                return zxerr_unknown;
            }
            pageStringExt(outVal, outValLen, tmpBuffer, sizeof(tmpBuffer), pageIdx, pageCount);
            break;
        }

        case 4:
            snprintf(outKey, outKeyLen, "Start");
            snprintf(outVal, outValLen, "%d", vault->vestingStart);
            break;

        case 5:
            snprintf(outKey, outKeyLen, "End");
            snprintf(outVal, outValLen, "%d", vault->vestingEnd);
            break;

        case 6:
            snprintf(outKey, outKeyLen, "Participants");
            snprintf(outVal, outValLen, "%d", vault->owner.participants);
            break;

        case 7:
            snprintf(outKey, outKeyLen, "Validators");
            snprintf(outVal, outValLen, "%d", vault->owner.approvers);
            break;

        default: {
            const uint8_t tmpDisplayIdx = displayIdx - 8;
            const uint8_t *pubkeyPtr = NULL;
            snprintf(outKey, outKeyLen, "Pubkey %d", tmpDisplayIdx);
            CHECK_ZXERR(getPublicKey(tmpDisplayIdx, vault->owner.internalIndex, vault->owner.keys, &pubkeyPtr))
            pageStringHex(outVal, outValLen, (const char *)pubkeyPtr, PUB_KEY_LENGTH, pageIdx, pageCount);
            break;
        }
    }

    return zxerr_ok;
}
