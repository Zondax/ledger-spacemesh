/** ******************************************************************************
 *  (c) 2019-2024 Zondax AG
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
 ******************************************************************************* */
import type Transport from '@ledgerhq/hw-transport'
import BaseApp, { BIP32Path, INSGeneric, processErrorResponse, processResponse } from '@zondax/ledger-js'

import { Account, VaultAccount, ResponseAddress, AccountType } from "./types";
import { P1_VALUES, PUBKEYLEN } from './consts'

import { ResponseSign, EdSigner } from './types'
import { ByteStream } from '@zondax/ledger-js/dist/byteStream';

const maxUint64 = BigInt("0xFFFFFFFFFFFFFFFF");
const maxUint32 = BigInt("0xFFFFFFFF");
const maxUint8 = BigInt("0xFF");
const pubKeyLength: number = 32;

export class SpaceMeshApp extends BaseApp {
  static _INS = {
    GET_VERSION: 0x00 as number,
    GET_ADDR: 0x01 as number,
    SIGN: 0x02 as number,

    GET_ADD_MULTISIG: 0x03 as number,
    GET_ADDR_VESTING: 0x04 as number,
    GET_ADDR_VAULT: 0x05 as number,
  }

  static _params = {
    cla: 0x45,
    ins: { ...SpaceMeshApp._INS } as INSGeneric,
    p1Values: { ONLY_RETRIEVE: 0x00 as 0, SHOW_ADDRESS_IN_DEVICE: 0x01 as 1 },
    chunkSize: 250,
    requiredPathLengths: [5],
  }

  constructor(transport: Transport) {
    super(transport, SpaceMeshApp._params)
    if (!this.transport) {
      throw new Error('Transport has not been defined')
    }
  }

  async getAddressAndPubKey(path: string, showAddrInDevice = false): Promise<ResponseAddress> {
    const bip44PathBuffer = this.serializePath(path)
    const p1 = showAddrInDevice ? P1_VALUES.SHOW_ADDRESS_IN_DEVICE : P1_VALUES.ONLY_RETRIEVE

    try {
      const responseBuffer = await this.transport.send(this.CLA, this.INS.GET_ADDR, p1, 0, bip44PathBuffer)

      const response = processResponse(responseBuffer)
      const pubkey = response.readBytes(PUBKEYLEN)
      const address = response.readBytes(response.length()).toString()

      return {
        pubkey,
        address,
      } as ResponseAddress

    } catch (e) {
      throw processErrorResponse(e)
    }
  }

  async getAddressMultisig(path: string, internalIndex: number, account: Account): Promise<ResponseAddress> {
    this.checkAccountsSanity(internalIndex, account);

    if (account.type !== AccountType.Multisig) {
      throw new Error('Invalid account type for multisig address');
    }

    const bs = new ByteStream()
    bs.appendUint8(internalIndex)
    bs.appendBytes(this.serializeAccount(account))
    const payload = bs.getCompleteBuffer();

    // [path|internalIndex|approvers|participants|[index|pubkeys]]
    const chunks = this.prepareChunks(path, payload);


    try {
      let response;
      for (let i = 0; i < chunks.length; i++) {
        response = await this.signSendChunk(this.INS.GET_ADDR_MULTISIG, i + 1, chunks.length, chunks[i]);
      }

      if (!response) {
        throw new Error('Failed to receive response from device');
      }

      return {
        pubkey: response.readBytes(PUBKEYLEN),
        address: response.getAvailableBuffer().toString(),
      } as ResponseAddress
 
    } catch (e) {
      throw processErrorResponse(e)
    }
  }

  async getAddressVesting(path: string, internalIndex: number, account: Account): Promise<ResponseAddress> {
    this.checkAccountsSanity(internalIndex, account);

    if (account.type !== AccountType.Vesting) {
      throw new Error('Invalid account type for vesting address');
    }

    const bs = new ByteStream()
    bs.appendUint8(internalIndex)
    bs.appendBytes(this.serializeAccount(account))
    const payload = bs.getCompleteBuffer();

    // [path|internalIndex|approvers|participants|[index|pubkeys]]
    const chunks = this.prepareChunks(path, payload);


    try {
      let response;
      for (let i = 0; i < chunks.length; i++) {
        response = await this.signSendChunk(this.INS.GET_ADDR_VESTING, i + 1, chunks.length, chunks[i]);
      }

      if (!response) {
        throw new Error('Failed to receive response from device');
      }

      return {
        pubkey: response.readBytes(PUBKEYLEN),
        address: response.getAvailableBuffer().toString(),
      } as ResponseAddress
 
    } catch (e) {
      throw processErrorResponse(e)
    }
  }

  async getAddressVault(path: string, internalIndex: number, account: VaultAccount, testMode = false): Promise<ResponseAddress> {

    if (!testMode) {
      this.checkAccountsSanity(internalIndex, account);
    }

    if (account.type !== AccountType.Vault) {
      throw new Error('Invalid account type for vesting address');
    }

    const payload = this.serializeVaultAccount(internalIndex, account);

    // [path | totalAmount | initialUnlockAmount | vestingStart | vestingEnd | internalIndex | approvers | participants [idx|pubkey] ]
    const chunks = this.prepareChunks(path, payload);

    try {
      let response;
      for (let i = 0; i < chunks.length; i++) {
        response = await this.signSendChunk(this.INS.GET_ADDR_VAULT, i + 1, chunks.length, chunks[i]);
      }

      if (!response) {
        throw new Error('Failed to receive response from device');
      }

      const pubkey = response.readBytes(PUBKEYLEN)
      const address = response.readBytes(response.length()).toString()

      return {
        pubkey,
        address,
      } as ResponseAddress
    } catch (e) {
      throw processErrorResponse(e)
    }
  }

  async sign(path: BIP32Path, blob: EdSigner): Promise<ResponseSign> {

    const payload = Buffer.concat([blob.prefix, Buffer.from([blob.domain]), blob.message]);
    const chunks = this.prepareChunks(path, payload);
    try {
      let signatureResponse = await this.signSendChunk(this.INS.SIGN, 1, chunks.length, chunks[0])

      for (let i = 1; i < chunks.length; i += 1) {
        signatureResponse = await this.signSendChunk(this.INS.SIGN, 1 + i, chunks.length, chunks[i])
      }
      return {
        signature: signatureResponse.readBytes(signatureResponse.length()),
      }

    } catch (e) {
      throw processErrorResponse(e)
    }
  }

  private getInstruction(id: AccountType): number {
    switch (id) {
      case AccountType.Wallet:
        return this.INS.GET_ADDR;
      case AccountType.Multisig:
        return this.INS.GET_MULTISIG_ADDR;
      case AccountType.Vesting:
        return this.INS.GET_VESTING_ADDR;
      case AccountType.Vault:
        return this.INS.GET_VAULT_ADDR;
    }
  }

  private checkAccountsSanity(internalIndex: number, account: Account) {
    const indices = new Set<number>()
    indices.add(internalIndex)
    for (const pubkey of account.pubkeys) {
      if (indices.has(pubkey.index)) {
        throw new Error(`Duplicate index ${pubkey.index} found in pubkeys array`)
      }
      indices.add(pubkey.index)

      if (pubkey.pubkey.length != pubKeyLength) {
        throw new Error(`Invalid pubkey size for ${pubkey.pubkey}`)
      }
    }

    for (let i = 0; i < indices.size; i++) {
      if (!indices.has(i)) {
        throw new Error(`Missing index ${i} in pubkeys array`)
      }
    }

    if (indices.size != account.participants) {
      throw new Error(`Pubkey quantity does not match the number of participants`)
    }

    if (account.approvers == 0) {
      throw new Error(`Approvers cannot be 0`)
    }

    if (account.participants < account.approvers) {
      throw new Error(`Approvers cannot exceed the number of participants`)
    }

    account.pubkeys.sort((a, b) => a.index - b.index);
  }

  private serializeVaultAccount(internalIndex: number, account: VaultAccount): Buffer {
    if (account.totalAmount > maxUint64 || account.initialUnlockAmount > maxUint64) {
      throw new Error(`Amount exceeds the maximum allowed value for uint64`);
    }
    if (account.vestingStart > maxUint32 || account.vestingEnd > maxUint32) {
      throw new Error(`Vesting exceeds the maximum allowed value for uint32`);
    }

    const bs = new ByteStream()

    bs.appendUint64(account.totalAmount)
    bs.appendUint64(account.initialUnlockAmount)
    bs.appendUint32(account.vestingStart)
    bs.appendUint32(account.vestingEnd)

    bs.appendUint8(internalIndex)
    bs.appendBytes(this.serializeAccount(account))

    return bs.getCompleteBuffer()
  }

  // FIXME: Why do we need an index here? Why not [count] + [pubkeys...]?
  private serializeAccount(account: Account): Buffer {

    if (account.approvers > maxUint8 || account.participants > maxUint8) {
      throw new Error(`Approvers or participants exceed the maximum allowed value for uint8`);
    }

    const b = new ByteStream()

    b.appendUint8(account.approvers)
    b.appendUint8(account.participants)

    for (const pubkey of account.pubkeys) {
      b.appendUint8(pubkey.index)
      b.appendBytes(pubkey.pubkey)
    }

    return b.getCompleteBuffer()
  }
}
