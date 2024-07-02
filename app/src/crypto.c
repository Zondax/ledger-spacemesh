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

extern address_request_t addr_request;

void logAccount(generic_account_t *account, pubkey_item_t *internalPubkey);

zxerr_t crypto_extractPublicKey(uint8_t *pubKey, uint16_t pubKeyLen) {
    if (pubKey == NULL || pubKeyLen < PUB_KEY_LENGTH) {
        return zxerr_invalid_crypto_settings;
    }
    cx_ecfp_public_key_t cx_publicKey;
    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[SK_LEN_25519] = {0};

    zxerr_t error = zxerr_unknown;

    // Generate keys
    CATCH_CXERROR(os_derive_bip32_with_seed_no_throw(HDW_ED25519_SLIP10, CX_CURVE_Ed25519, hdPath, HDPATH_LEN_DEFAULT,
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
    CATCH_CXERROR(os_derive_bip32_with_seed_no_throw(HDW_ED25519_SLIP10, CX_CURVE_Ed25519, hdPath, HDPATH_LEN_DEFAULT,
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

zxerr_t crypto_fillAddress(uint8_t *outBuffer, uint16_t outBufferLen, uint16_t *addrResponseLen) {
    // Clear up first
    if (outBuffer == NULL || addrResponseLen == NULL) {
        return zxerr_out_of_bounds;
    }

    MEMZERO(outBuffer, outBufferLen);
    *addrResponseLen = 0;

    if (outBufferLen < sizeof(apdu_address_response_t)) {
        return zxerr_unknown;
    }

    apdu_address_response_t *resp = (apdu_address_response_t *)outBuffer;

    // Get pubkey and account
    pubkey_item_t internalPubkey = {.pubkey = {0}, .index = 0};
    CHECK_ZXERR(crypto_extractPublicKey(internalPubkey.pubkey, sizeof(internalPubkey.pubkey)));
    MEMCPY(resp->pubkey, internalPubkey.pubkey, PUB_KEY_LENGTH);

    // Bech32 encoding on account
    uint8_t address_encoded[MAX_ADDRESS_LENGTH] = {0};
    CHECK_ZXERR(crypto_encodeAccountPubkey(address_encoded, sizeof(address_encoded), &internalPubkey, NULL, WALLET));

    const char *hrp = calculate_hrp();
    CHECK_ZXERR(
        bech32EncodeFromBytes(resp->address_bech32, 64, hrp, address_encoded, ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32));

    *addrResponseLen = sizeof(resp->pubkey) + strnlen((const char *)resp->address_bech32, MAX_ADDRESS_LENGTH);

    ZEMU_LOGF(50, "addr_len: %d", *addrResponseLen);
    return zxerr_ok;
}

zxerr_t crypto_fillAddressMultisigOrVesting(uint8_t *outBuffer, uint16_t outBufferLen, uint16_t *addrResponseLen) {
    if (outBuffer == NULL || addrResponseLen == NULL) {
        return zxerr_out_of_bounds;
    }

    // Clear up first in case returned error is ignored
    MEMZERO(outBuffer, outBufferLen);
    *addrResponseLen = 0;

    if (outBufferLen < sizeof(apdu_address_response_t)) {
        return zxerr_buffer_too_small;
    }

    apdu_address_response_t *resp = (apdu_address_response_t *)outBuffer;

    if (addr_request.account_type != MULTISIG && addr_request.account_type != VESTING) {
        return zxerr_invalid_crypto_settings;
    }

    // Get internal Pubkey
    pubkey_item_t internalPubkey = {.pubkey = {0}, .index = addr_request.internalIndex};
    CHECK_ZXERR(crypto_extractPublicKey(internalPubkey.pubkey, sizeof(internalPubkey.pubkey)));

    logAccount(addr_request.account, &internalPubkey);

    uint8_t address[MAX_ADDRESS_LENGTH] = {0};
    CHECK_ZXERR(crypto_encodeAccountPubkey(address, sizeof(address), &internalPubkey, addr_request.account,
                                           addr_request.account_type));

    // Copy internal pubkey in the buffer
    const char *hrp = calculate_hrp();
    MEMCPY(resp->pubkey, internalPubkey.pubkey, PUB_KEY_LENGTH);
    CHECK_ZXERR(bech32EncodeFromBytes(resp->address_bech32, 64, hrp, address, ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32));

    *addrResponseLen = PUB_KEY_LENGTH + strnlen(resp->address_bech32, sizeof(resp->address_bech32));

    return zxerr_ok;
}

zxerr_t crypto_fillAddressVault(uint8_t *outBuffer, uint16_t outBufferLen, uint16_t *addrResponseLen) {
    if (outBuffer == NULL || addrResponseLen == NULL) {
        return zxerr_out_of_bounds;
    }

    // Clear up first in case returned error is ignored
    MEMZERO(outBuffer, outBufferLen);
    *addrResponseLen = 0;

    if (outBufferLen < sizeof(apdu_address_response_t)) {
        return zxerr_unknown;
    }

    apdu_address_response_t *resp = (apdu_address_response_t *)outBuffer;

    if (addr_request.account_type != VAULT) {
        return zxerr_invalid_crypto_settings;
    }

    // Get internal Pubkey
    pubkey_item_t internalPubkey = {.pubkey = {0}, .index = addr_request.internalIndex};
    CHECK_ZXERR(crypto_extractPublicKey(internalPubkey.pubkey, sizeof(internalPubkey.pubkey)));

    logAccount(&addr_request.vault_account->owner, &internalPubkey);

    uint8_t address[MAX_ADDRESS_LENGTH] = {0};
    CHECK_ZXERR(crypto_encodeVaultPubkey(address, sizeof(address), &internalPubkey, addr_request.vault_account));

    const char *hrp = calculate_hrp();
    MEMCPY(resp->pubkey, internalPubkey.pubkey, PUB_KEY_LENGTH);
    CHECK_ZXERR(bech32EncodeFromBytes(resp->address_bech32, 64, hrp, address, ADDRESS_LENGTH, 1, BECH32_ENCODING_BECH32));

    *addrResponseLen = PUB_KEY_LENGTH + strnlen(resp->address_bech32, sizeof(resp->address_bech32));

    return zxerr_ok;
}

void logAccount(generic_account_t *account, pubkey_item_t *internalPubkey) {
#ifndef APP_TESTING
    (void)account;
    (void)internalPubkey;
#else
    if (account == NULL) {
        return;
    }
    char print[100] = {0};
    uint8_t index = 0;
    for (int i = 0; i < account->participants; i++) {
        if (i == internalPubkey->index) {
            array_to_hexstr(print, sizeof(print), internalPubkey->pubkey, 32);
            ZEMU_LOGF(100, "pubkey [%d] = %s\n", internalPubkey->index, print);
        } else {
            array_to_hexstr(print, sizeof(print), account->keys[index].pubkey, 32);
            ZEMU_LOGF(100, "pubkey [%d] = %s\n", account->keys[index].index, print);
            index++;
        }
    }
    ZEMU_LOGF(100, "approvers: %d; participants: %d\n", account->approvers, account->participants);
#endif
}
