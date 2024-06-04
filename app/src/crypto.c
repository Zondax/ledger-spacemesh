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
#include "crypto.h"

#include "bech32.h"
#include "coin.h"
#include "crypto_helper.h"
#include "cx.h"
#include "zxformat.h"
#include "zxmacros.h"

#define BUFF_INTERNAL_PUBKEY_INDEX 0
#define BUFF_APPROVERS_INDEX 1
#define BUFF_PARTICIPANTS_INDEX 2
#define BUFF_FIRST_ACCOUNT_INDEX 3
#define BUFF_START_ACCOUNT_PUBKEY_INDEX 4
#define VAULT_INFO_SIZE 24
#define BUFF_ACCOUNT_INFO_LENGTH (PUB_KEY_LENGTH + 1)

uint32_t hdPath[HDPATH_LEN_DEFAULT];

// #{TODO} --> Check pubkey and sign methods

zxerr_t createIndexesArray(const uint8_t *buffer, const uint16_t pubkeysBuffSize, uint8_t *indexes, uint8_t *indexesQty);
zxerr_t createAccount(const uint8_t *buffer, const uint8_t pubkeyQty, const uint8_t internalPubkeyIndex,
                      const uint8_t *pubkeyIndexes, const uint8_t id, account_t *account);
zxerr_t checkAccountsSanity(const uint8_t *buffer, uint8_t *indexes, uint8_t pubkeyQty);
uint32_t load32(const uint8_t *src);
uint64_t load64(const uint8_t *src);
void logAccount(account_t *account, uint8_t pubkeyQty);

zxerr_t crypto_extractPublicKey(uint8_t *pubKey, uint16_t pubKeyLen) {
    if (pubKey == NULL || pubKeyLen < PK_LEN_25519) {
        return zxerr_invalid_crypto_settings;
    }
    cx_ecfp_public_key_t cx_publicKey;
    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[SK_LEN_25519] = {0};

    zxerr_t error = zxerr_unknown;

    // Generate keys
    CATCH_CXERROR(os_derive_bip32_with_seed_no_throw(HDW_NORMAL, CX_CURVE_Ed25519, hdPath, HDPATH_LEN_DEFAULT,
                                                     privateKeyData, NULL, NULL, 0));

    CATCH_CXERROR(cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519, privateKeyData, 32, &cx_privateKey));
    CATCH_CXERROR(cx_ecfp_init_public_key_no_throw(CX_CURVE_Ed25519, NULL, 0, &cx_publicKey));
    CATCH_CXERROR(cx_ecfp_generate_pair_no_throw(CX_CURVE_Ed25519, &cx_publicKey, &cx_privateKey, 1));
    for (unsigned int i = 0; i < PK_LEN_25519; i++) {
        pubKey[i] = cx_publicKey.W[64 - i];
    }

    if ((cx_publicKey.W[PK_LEN_25519] & 1) != 0) {
        pubKey[31] |= 0x80;
    }
    error = zxerr_ok;

catch_cx_error:
    MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
    MEMZERO(privateKeyData, sizeof(privateKeyData));

    if (error != zxerr_ok) {
        MEMZERO(pubKey, pubKeyLen);
    }
    return error;
}

zxerr_t crypto_sign(uint8_t *signature, uint16_t signatureMaxlen, const uint8_t *message, uint16_t messageLen) {
    if (signature == NULL || message == NULL || signatureMaxlen < ED25519_SIGNATURE_SIZE || messageLen == 0) {
        return zxerr_invalid_crypto_settings;
    }

    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[SK_LEN_25519] = {0};

    zxerr_t error = zxerr_unknown;
    // Generate keys
    CATCH_CXERROR(os_derive_bip32_with_seed_no_throw(HDW_NORMAL, CX_CURVE_Ed25519, hdPath, HDPATH_LEN_DEFAULT,
                                                     privateKeyData, NULL, NULL, 0));

    CATCH_CXERROR(cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519, privateKeyData, SCALAR_LEN_ED25519, &cx_privateKey));

    // Sign
    CATCH_CXERROR(cx_eddsa_sign_no_throw(&cx_privateKey, CX_SHA512, message, messageLen, signature, signatureMaxlen));

    error = zxerr_ok;

catch_cx_error:
    MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
    MEMZERO(privateKeyData, sizeof(privateKeyData));

    if (error != zxerr_ok) {
        MEMZERO(signature, signatureMaxlen);
    }

    return error;
}

zxerr_t crypto_fillAddress(uint8_t *buffer, uint16_t bufferLen, uint16_t *addrResponseLen) {
    const bool mainnet = hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT;
    const uint8_t outAddressLen = mainnet ? MIN_MAIN_ADDRESS_BUFFER_LEN : MIN_TEST_ADDRESS_BUFFER_LEN;

    if (bufferLen < PUB_KEY_LENGTH + outAddressLen) {
        return zxerr_unknown;
    }

    MEMZERO(buffer, bufferLen);
    CHECK_ZXERR(crypto_extractPublicKey(buffer, bufferLen))

    account_t walletccount;
    MEMCPY(walletccount.keys[0], buffer, PUB_KEY_LENGTH);

    walletccount.participants = 1;
    walletccount.id = WALLET;

    uint8_t address[64] = {0};
    uint8_t offset = 0;
    const char *hrp = mainnet ? "sm" : "stest";
    CHECK_ZXERR(crypto_encodeAccountPubkey(address, sizeof(address), &walletccount, &offset, mainnet));
    zxerr_t err = bech32EncodeFromBytes((char *)buffer + PUB_KEY_LENGTH, 64, hrp, address + offset, ADDRESS_LENGTH, 1,
                                        BECH32_ENCODING_BECH32);

    if (err != zxerr_ok) {
        MEMZERO(buffer, bufferLen);
        return zxerr_encoding_failed;
    }

    *addrResponseLen = PUB_KEY_LENGTH + outAddressLen;
    return zxerr_ok;
}

zxerr_t crypto_fillMultisigAddress(const uint8_t *buffer, const uint16_t bufferLen, uint16_t *addrResponseLen,
                                   uint8_t accountId) {
    if (buffer == NULL || addrResponseLen == NULL) {
        return zxerr_invalid_crypto_settings;
    }
    const bool mainnet = (hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT);
    const uint8_t outAddressLen = mainnet ? MIN_MAIN_ADDRESS_BUFFER_LEN : MIN_TEST_ADDRESS_BUFFER_LEN;
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    if (IO_APDU_BUFFER_SIZE < PUB_KEY_LENGTH + outAddressLen) {
        return zxerr_unknown;
    }

    uint8_t pubkeysBuffSize = bufferLen - BUFF_FIRST_ACCOUNT_INDEX;
    uint8_t pubkeyIndexes[MAX_MULTISIG_PUB_KEY] = {0};
    uint8_t pubkeyQty;
    CHECK_ZXERR(createIndexesArray(buffer, pubkeysBuffSize, pubkeyIndexes, &pubkeyQty));
    CHECK_ZXERR(checkAccountsSanity(buffer, pubkeyIndexes, pubkeyQty))

    zxerr_t error = crypto_extractPublicKey(G_io_apdu_buffer, PUB_KEY_LENGTH);
    if (error != zxerr_ok) {
        MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
        return error;
    }

    uint8_t internalPubkeyIndex = buffer[BUFF_INTERNAL_PUBKEY_INDEX];
    account_t account;
    CHECK_ZXERR(createAccount(buffer, pubkeyQty, internalPubkeyIndex, pubkeyIndexes, accountId, &account));
    logAccount(&account, pubkeyQty);

    uint8_t address[64] = {0};
    uint8_t addrOffset = 0;
    const char *hrp = mainnet ? "sm" : "stest";
    error = crypto_encodeAccountPubkey(address, sizeof(address), &account, &addrOffset, mainnet);
    if (error != zxerr_ok) {
        MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
        return error;
    }
    error = bech32EncodeFromBytes((char *)G_io_apdu_buffer + PUB_KEY_LENGTH, 64, hrp, address + addrOffset, ADDRESS_LENGTH,
                                  1, BECH32_ENCODING_BECH32);
    if (error != zxerr_ok) {
        MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
        return error;
    }

    *addrResponseLen = PUB_KEY_LENGTH + outAddressLen;
    return zxerr_ok;
}

zxerr_t crypto_fillVaultAddress(const uint8_t *buffer, const uint16_t bufferLen, uint16_t *addrResponseLen) {
    if (buffer == NULL || addrResponseLen == NULL) {
        return zxerr_invalid_crypto_settings;
    }

    const bool mainnet = (hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT);
    const uint8_t outAddressLen = mainnet ? MIN_MAIN_ADDRESS_BUFFER_LEN : MIN_TEST_ADDRESS_BUFFER_LEN;
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    if (IO_APDU_BUFFER_SIZE < PUB_KEY_LENGTH + outAddressLen) {
        return zxerr_unknown;
    }

    uint8_t pubkeysBuffSize = bufferLen - (BUFF_FIRST_ACCOUNT_INDEX + VAULT_INFO_SIZE);
    uint8_t pubkeyIndexes[MAX_MULTISIG_PUB_KEY] = {0};
    uint8_t pubkeyQty;
    CHECK_ZXERR(createIndexesArray(buffer, pubkeysBuffSize, pubkeyIndexes, &pubkeyQty));
    CHECK_ZXERR(checkAccountsSanity(buffer, pubkeyIndexes, pubkeyQty))

    zxerr_t error = crypto_extractPublicKey(G_io_apdu_buffer, PUB_KEY_LENGTH);
    if (error != zxerr_ok) {
        MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
        return error;
    }

    uint8_t internalPubkeyIndex = buffer[BUFF_INTERNAL_PUBKEY_INDEX];
    vault_account_t vaultAccount;
    account_t account;
    CHECK_ZXERR(createAccount(buffer, pubkeyQty, internalPubkeyIndex, pubkeyIndexes, VESTING, &account));
    vaultAccount.owner = account;
    vaultAccount.totalAmount = load64(buffer + bufferLen - 24);
    vaultAccount.initialUnlockAmount = load64(buffer + bufferLen - 16);
    vaultAccount.vestingStart = load32(buffer + bufferLen - 8);
    vaultAccount.vestingEnd = load32(buffer + bufferLen - 4);
    vaultAccount.id = VAULT;
    logAccount(&vaultAccount.owner, pubkeyQty);

    uint8_t address[64] = {0};
    uint8_t addrOffset = 0;
    const char *hrp = mainnet ? "sm" : "stest";
    error = crypto_encodeVaultPubkey(address, sizeof(address), &vaultAccount, &addrOffset, mainnet);
    if (error != zxerr_ok) {
        MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
        return error;
    }
    error = bech32EncodeFromBytes((char *)G_io_apdu_buffer + PUB_KEY_LENGTH, 64, hrp, address + addrOffset, ADDRESS_LENGTH,
                                  1, BECH32_ENCODING_BECH32);
    if (error != zxerr_ok) {
        MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
        return error;
    }

    *addrResponseLen = PUB_KEY_LENGTH + outAddressLen;
    return zxerr_ok;
}

zxerr_t createIndexesArray(const uint8_t *buffer, const uint16_t pubkeysBuffSize, uint8_t *indexes, uint8_t *indexesQty) {
    if (buffer == NULL || indexes == NULL || indexesQty == NULL) {
        return zxerr_invalid_crypto_settings;
    }
    if (pubkeysBuffSize % BUFF_ACCOUNT_INFO_LENGTH != 0) {
        return zxerr_invalid_crypto_settings;
    }

    uint8_t auxPubkeyQty = pubkeysBuffSize / PUB_KEY_LENGTH;
    if (auxPubkeyQty >= MAX_MULTISIG_PUB_KEY) {
        return zxerr_invalid_crypto_settings;
    }

    uint16_t offsetIndex = BUFF_FIRST_ACCOUNT_INDEX;
    for (int i = 0; i < auxPubkeyQty; i++) {
        indexes[i] = buffer[offsetIndex];
        offsetIndex += BUFF_ACCOUNT_INFO_LENGTH;
    }
    uint8_t internalPubkeyIndex = buffer[BUFF_INTERNAL_PUBKEY_INDEX];
    indexes[auxPubkeyQty] = internalPubkeyIndex;
    *indexesQty = auxPubkeyQty + 1;
    return zxerr_ok;
}

zxerr_t createAccount(const uint8_t *buffer, const uint8_t pubkeyQty, const uint8_t internalPubkeyIndex,
                      const uint8_t *pubkeyIndexes, const uint8_t id, account_t *account) {
    if (buffer == NULL || pubkeyIndexes == NULL || account == NULL) {
        return zxerr_invalid_crypto_settings;
    }
    uint16_t offset = BUFF_START_ACCOUNT_PUBKEY_INDEX;
    for (int i = 0; i < pubkeyQty; i++) {
        if (internalPubkeyIndex == pubkeyIndexes[i]) {
            MEMCPY(account->keys[pubkeyIndexes[i]], G_io_apdu_buffer, PUB_KEY_LENGTH);
        } else {
            MEMCPY(account->keys[pubkeyIndexes[i]], buffer + offset, PUB_KEY_LENGTH);
            offset += BUFF_ACCOUNT_INFO_LENGTH;
        }
    }
    account->approvers = buffer[BUFF_APPROVERS_INDEX];
    account->participants = buffer[BUFF_PARTICIPANTS_INDEX];
    account->id = id;

    return zxerr_ok;
}

zxerr_t checkAccountsSanity(const uint8_t *buffer, uint8_t *indexes, uint8_t pubkeyQty) {
    bool exist[MAX_MULTISIG_PUB_KEY] = {false};
    for (uint8_t i = 0; i < pubkeyQty; i++) {
        uint8_t index = indexes[i];
        if (index >= pubkeyQty || exist[index]) {
            return zxerr_invalid_crypto_settings;
        }
        exist[index] = true;
    }

    uint8_t participants = buffer[BUFF_PARTICIPANTS_INDEX];
    if (participants != pubkeyQty) {
        return zxerr_invalid_crypto_settings;
    }

    return zxerr_ok;
}

uint32_t load32(const uint8_t *src) {
    uint32_t w;
    MEMCPY(&w, src, sizeof w);
    return w;
}

uint64_t load64(const uint8_t *src) {
    uint64_t w;
    MEMCPY(&w, src, sizeof w);
    return w;
}

void logAccount(account_t *account, uint8_t pubkeyQty) {
    if (account == NULL) {
        return;
    }
    for (int i = 0; i < pubkeyQty; i++) {
        char print[100] = {0};
        array_to_hexstr(print, sizeof(print), account->keys[i], 32);
        ZEMU_LOGF(100, "pubkey [%d] = %s\n", i, print);
    }
    ZEMU_LOGF(100, "approvers: %d; participants: %d\n", account->approvers, account->participants);
}

// TODO: Move this function
const char *parser_getApiErrorDescription(zxerr_t err) {
    switch (err) {
        case zxerr_unknown:
            return "error unknown";
        case zxerr_ok:
            return "No error";
        case zxerr_no_data:
            return "No more data";
        case zxerr_buffer_too_small:
            return "Buffer too small";
        case zxerr_out_of_bounds:
            return "Out of bound";
        case zxerr_encoding_failed:
            return "Encoding failed";
        case zxerr_invalid_crypto_settings:
            return "Invalid crypto settings";
        case zxerr_ledger_api_error:
            return "Api error";

        default:
            return "Unrecognized error code";
    }
}
