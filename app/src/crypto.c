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

#define BUFF_FIRST_ACCOUNT_INDEX 3
#define VAULT_INFO_SIZE 24
#define BUFF_ACCOUNT_INFO_LENGTH (PUB_KEY_LENGTH + 1)

uint32_t hdPath[HDPATH_LEN_DEFAULT];

void logAccount(account_t *account);

zxerr_t crypto_extractPublicKey(uint8_t *pubKey, uint16_t pubKeyLen) {
    if (pubKey == NULL || pubKeyLen < PUB_KEY_LENGTH) {
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
    for (unsigned int i = 0; i < PUB_KEY_LENGTH; i++) {
        pubKey[i] = cx_publicKey.W[64 - i];
    }

    if ((cx_publicKey.W[PUB_KEY_LENGTH] & 1) != 0) {
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

    CATCH_CXERROR(cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519, privateKeyData, 32, &cx_privateKey));

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
    if (bufferLen < PUB_KEY_LENGTH + MIN_TEST_ADDRESS_BUFFER_LEN) {
        return zxerr_unknown;
    }

    MEMZERO(buffer, bufferLen);

    // Get pubkey and account
    pubkey_t internalPubkey = {0};
    CHECK_ZXERR(crypto_extractPublicKey(internalPubkey.pubkey, sizeof(internalPubkey.pubkey)));

    // Bech32 encoding on account
    uint8_t address[MIN_TEST_ADDRESS_BUFFER_LEN] = {0};
    const bool mainnet = hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT;
    const char *hrp = mainnet ? "sm" : "stest";
    CHECK_ZXERR(crypto_encodeAccountPubkey(address, sizeof(address), &internalPubkey, NULL, WALLET));
    zxerr_t err =
        bech32EncodeFromBytes((char *)buffer + PUB_KEY_LENGTH, 64, hrp, address, ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32);

    if (err != zxerr_ok) {
        MEMZERO(buffer, bufferLen);
        return zxerr_encoding_failed;
    }

    // Copy pubkey in the buffer
    MEMCPY(buffer, internalPubkey.pubkey, PUB_KEY_LENGTH);
    const uint8_t outAddressLen = mainnet ? MIN_MAIN_ADDRESS_BUFFER_LEN : MIN_TEST_ADDRESS_BUFFER_LEN;

    *addrResponseLen = PUB_KEY_LENGTH + outAddressLen;
    return zxerr_ok;
}

zxerr_t crypto_fillAddressMultisigOrVesting(const uint8_t *buffer, const uint16_t bufferLen, uint16_t *addrResponseLen,
                                            account_type_e account_type) {
    if (buffer == NULL || addrResponseLen == NULL) {
        return zxerr_invalid_crypto_settings;
    }

    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    if (IO_APDU_BUFFER_SIZE < PUB_KEY_LENGTH + MIN_TEST_ADDRESS_BUFFER_LEN) {
        return zxerr_unknown;
    }

    uint8_t pubkeysBuffSize = bufferLen - BUFF_FIRST_ACCOUNT_INDEX;
    // Ensure the buffer size is a multiple of BUFF_ACCOUNT_INFO_LENGTH
    if (pubkeysBuffSize % BUFF_ACCOUNT_INFO_LENGTH != 0) {
        return zxerr_invalid_crypto_settings;
    }

    // [internalIndex | approvers | participants [idx|pubkey]]
    account_t *account = (account_t *)buffer;

    // Validate the number of public keys
    if ((pubkeysBuffSize / BUFF_ACCOUNT_INFO_LENGTH) + 1 != account->participants) {
        return zxerr_invalid_crypto_settings;
    }

    // validate account
    uint8_t indexAux = 0;
    for (int i = 0; i < account->participants; i++) {
        if (i == account->internalIndex) {
            continue;
        }
        if (i == account->keys[indexAux].index) {
            indexAux++;
        } else {
            return zxerr_invalid_crypto_settings;
        }
    }
    logAccount(account);

    // Get internal Pubkey
    pubkey_t internalPubkey = {0};
    CHECK_ZXERR(crypto_extractPublicKey(internalPubkey.pubkey, sizeof(internalPubkey.pubkey)));

    uint8_t address[MIN_TEST_ADDRESS_BUFFER_LEN] = {0};
    const bool mainnet = hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT;
    const char *hrp = mainnet ? "sm" : "stest";
    CHECK_ZXERR(crypto_encodeAccountPubkey(address, sizeof(address), &internalPubkey, account, account_type));

    zxerr_t error = bech32EncodeFromBytes((char *)G_io_apdu_buffer + PUB_KEY_LENGTH, 64, hrp, address, ADDRESS_LENGTH, 1,
                                          BECH32_ENCODING_BECH32);

    if (error != zxerr_ok) {
        MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
        return error;
    }

    // Copy internal pubkey in the buffer
    MEMCPY(G_io_apdu_buffer, internalPubkey.pubkey, PUB_KEY_LENGTH);

    const uint8_t outAddressLen = mainnet ? MIN_MAIN_ADDRESS_BUFFER_LEN : MIN_TEST_ADDRESS_BUFFER_LEN;
    *addrResponseLen = PUB_KEY_LENGTH + outAddressLen;
    return zxerr_ok;
}

zxerr_t crypto_fillAddressVault(const uint8_t *buffer, const uint16_t bufferLen, uint16_t *addrResponseLen) {
    if (buffer == NULL || addrResponseLen == NULL) {
        return zxerr_invalid_crypto_settings;
    }

    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    if (IO_APDU_BUFFER_SIZE < PUB_KEY_LENGTH + MIN_TEST_ADDRESS_BUFFER_LEN) {
        return zxerr_unknown;
    }

    uint8_t pubkeysBuffSize = bufferLen - (BUFF_FIRST_ACCOUNT_INDEX + VAULT_INFO_SIZE);
    // Ensure the buffer size is a multiple of BUFF_ACCOUNT_INFO_LENGTH
    if (pubkeysBuffSize % BUFF_ACCOUNT_INFO_LENGTH != 0) {
        return zxerr_invalid_crypto_settings;
    }

    // [totalAmount | initialUnlockAmount | vestingStart | vestingEnd | internalIndex | approvers | participants |
    // [idx|pubkey]]
    vault_account_t *vaultAccount = (vault_account_t *)buffer;

    // Validate the number of public keys
    if ((pubkeysBuffSize / BUFF_ACCOUNT_INFO_LENGTH) + 1 != vaultAccount->owner.participants) {
        return zxerr_invalid_crypto_settings;
    }

    // create account
    uint8_t indexAux = 0;
    for (int i = 0; i < vaultAccount->owner.participants; i++) {
        if (i == vaultAccount->owner.internalIndex) {
            continue;
        }
        if (i == vaultAccount->owner.keys[indexAux].index) {
            indexAux++;
        } else {
            return zxerr_invalid_crypto_settings;
        }
    }
    logAccount(&vaultAccount->owner);

    // Get internal Pubkey
    pubkey_t internalPubkey = {0};
    CHECK_ZXERR(crypto_extractPublicKey(internalPubkey.pubkey, sizeof(internalPubkey.pubkey)));

    uint8_t address[MIN_TEST_ADDRESS_BUFFER_LEN] = {0};
    const bool mainnet = hdPath[0] == HDPATH_0_DEFAULT && hdPath[1] == HDPATH_1_DEFAULT;
    const char *hrp = mainnet ? "sm" : "stest";
    CHECK_ZXERR(crypto_encodeVaultPubkey(address, sizeof(address), &internalPubkey, vaultAccount, mainnet));

    zxerr_t error = bech32EncodeFromBytes((char *)G_io_apdu_buffer + PUB_KEY_LENGTH, 64, hrp, address, ADDRESS_LENGTH, 1,
                                          BECH32_ENCODING_BECH32);
    if (error != zxerr_ok) {
        MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
        return error;
    }

    // Copy internal pubkey in the buffer
    MEMCPY(G_io_apdu_buffer, internalPubkey.pubkey, PUB_KEY_LENGTH);

    const uint8_t outAddressLen = mainnet ? MIN_MAIN_ADDRESS_BUFFER_LEN : MIN_TEST_ADDRESS_BUFFER_LEN;
    *addrResponseLen = PUB_KEY_LENGTH + outAddressLen;
    return zxerr_ok;
}

void logAccount(account_t *account) {
    (void)account;
#ifdef APP_TESTING
    if (account == NULL) {
        return;
    }
    char print[100] = {0};
    for (int i = 0; i < account->participants; i++) {
        array_to_hexstr(print, sizeof(print), account->keys[i].pubkey, 32);
        ZEMU_LOGF(100, "pubkey [%d] = %s\n", account->keys[i].index, print);
    }
    ZEMU_LOGF(100, "approvers: %d; participants: %d\n", account->approvers, account->participants);
#endif
}
