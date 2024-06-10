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

#include <stdio.h>

#include "app_mode.h"
#include "coin.h"
#include "crypto.h"
#include "zxerror.h"
#include "zxformat.h"
#include "zxmacros.h"
#include "parser_txdef.h"
#include "os.h"
#include "addr.h"
#include "tx.h"

zxerr_t wallet_getNumItems(uint8_t *num_items) {
    zemu_log_stack("addr_getNumItems");
    *num_items = 1;
    if (app_mode_expert()) {
        *num_items = 2;
    }
    return zxerr_ok;
}

zxerr_t wallet_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen, uint8_t pageIdx,
                     uint8_t *pageCount) {
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

static zxerr_t getPublicKey(const uint8_t index, multisig_t *multisig, const uint8_t** pubkeyPtr) {
    if (multisig == NULL || pubkeyPtr == NULL) {
        return zxerr_no_data;
    }
    if (index == multisig->internalIndex) {
        *pubkeyPtr = G_io_apdu_buffer;
        return zxerr_ok;
    }

    // On the tx_buffer we have only external pubkeys
    const uint8_t tmpIndex = index > multisig->internalIndex ? (index - 1) : index;
    *pubkeyPtr = multisig->keys[tmpIndex].pubkey;
    return zxerr_ok;
}

zxerr_t multisigVesting_getNumItems(uint8_t *num_items) {
    ZEMU_LOGF(50, "multisigVesting_getNumItems\n");

    const uint8_t fixedFields = 3; // Participants, Approvers, internal pubkey
    const uint8_t totalExternalPubkeys = (tx_get_buffer_length() - fixedFields) / 33;

    // Everything from above + address + path
    *num_items = totalExternalPubkeys + fixedFields + 2;
    return zxerr_ok;
}

zxerr_t multisigVesting_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen, char *outVal, uint16_t outValLen, uint8_t pageIdx,
                     uint8_t *pageCount) {
    ZEMU_LOGF(50, "[addr_getItem] %d/%d\n", displayIdx, pageIdx)
    uint8_t numItems = 0;
    multisigVesting_getNumItems(&numItems);
    if (displayIdx >= numItems) {
        return zxerr_no_data;
    }

    // [internalIndex | approvers | participants [idx|pubkey]]
    multisig_t *multisig = (multisig_t*) tx_get_buffer();

    switch (displayIdx) {
        case 0:
            //TODO: replace Multisig / Vesting depending on ???
            snprintf(outKey, outKeyLen, "Multisig");
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
            snprintf(outVal, outValLen, "%d", multisig->participants);
            return zxerr_ok;

        case 3:
            snprintf(outKey, outKeyLen, "Validators");
            snprintf(outVal, outValLen, "%d", multisig->approvers);
            return zxerr_ok;

        default: {
            const uint8_t tmpDisplayIdx = displayIdx - 4;
            const uint8_t *pubkeyPtr = NULL;
            snprintf(outKey, outKeyLen, "Pubkey %d", tmpDisplayIdx);
            CHECK_ZXERR(getPublicKey(tmpDisplayIdx, multisig, &pubkeyPtr))
            pageStringHex(outVal, outValLen, (const char*) pubkeyPtr, PUB_KEY_LENGTH, pageIdx, pageCount);
            return zxerr_ok;
        }
    }

    return zxerr_no_data;
}

zxerr_t vault_getNumItems(uint8_t *num_items) {
    ZEMU_LOGF(50, "vault_getNumItems\n");

    const uint8_t fixedFields = 3; // Participants, Approvers, internal pubkey
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
    vault_t *vault = (vault_t*) tx_get_buffer();
    char tmpBuffer[30] = {0};
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Vault");
            snprintf(outVal, outValLen, "%d", vault->internalIndex);
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
            snprintf(outVal, outValLen, "%d", vault->participants);
            break;

        case 7:
            snprintf(outKey, outKeyLen, "Validators");
            snprintf(outVal, outValLen, "%d", vault->approvers);
            break;

        default: {
            // const uint8_t tmpDisplayIdx = displayIdx - 8;
            // const uint8_t *pubkeyPtr = NULL;
            // snprintf(outKey, outKeyLen, "Pubkey %d", tmpDisplayIdx);
            // CHECK_ZXERR(getPublicKey(tmpDisplayIdx, &vault->keys, &pubkeyPtr))
            // pageStringHex(outVal, outValLen, (const char*) pubkeyPtr, PUB_KEY_LENGTH, pageIdx, pageCount);
            break;
        }
    }

    return zxerr_ok;
}
