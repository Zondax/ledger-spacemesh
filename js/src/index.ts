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

const maxUint64 = BigInt("0xFFFFFFFFFFFFFFFF");
const maxUint32 = BigInt("0xFFFFFFFF");
const maxUint8 = BigInt("0xFF");
const pubKeyLength: number = 32;

export class SpaceMeshApp extends BaseApp {
  static _INS = {
    GET_VERSION: 0x00 as number,
    GET_ADDR: 0x01 as number,
    SIGN: 0x02 as number,
    GET_MULTISIG_ADDR: 0x03 as number,
    GET_VESTING_ADDR: 0x04 as number,
    GET_VAULT_ADDR: 0x05 as number,
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

  async getInfoMultisigVestingAccount(path: string, internalIndex: number, account: Account): Promise<ResponseAddress> {
    this.checkAccountsSanity(internalIndex, account);
    const serializedAccount = this.serializeAccount(account);
    const payload = Buffer.concat([Buffer.from([internalIndex]), serializedAccount]);

    // [path|internalIndex|approvers|participants|[index|pubkeys]]
    const chunks = this.prepareChunks(path, payload);

    try {
      // // Instruction matches Account type
      // WALLET = 0
      // MULTISIG = 1
      let response = await this.signSendChunk(this.getInstruction(account.id), 1, chunks.length, chunks[0])
      for (let i = 1; i < chunks.length; i += 1) {
        response = await this.signSendChunk(this.getInstruction(account.id), 1 + i, chunks.length, chunks[i])
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

  async getInfoVaultAccount(path: string, internalIndex: number, vaultAccount: VaultAccount, testMode = false): Promise<ResponseAddress> {
    if (!testMode) {
      this.checkAccountsSanity(internalIndex, vaultAccount.owner);
    }
    const payload = this.serializeVaultAccount(internalIndex, vaultAccount);

    // [path | totalAmount | initialUnlockAmount | vestingStart | vestingEnd | internalIndex | approvers | participants [idx|pubkey] ]
    const chunks = this.prepareChunks(path, payload);

    try {
      let response = await this.signSendChunk(this.getInstruction(vaultAccount.id), 1, chunks.length, chunks[0])
      for (let i = 1; i < chunks.length; i += 1) {
        response = await this.signSendChunk(this.getInstruction(vaultAccount.id), 1 + i, chunks.length, chunks[i])
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
    // TODO: if P2 is needed, use `sendGenericChunk`
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
    const serializedOwnerAccount = this.serializeAccount(account.owner);
    let buff = Buffer.alloc(24)

    if (account.totalAmount > maxUint64 || account.initialUnlockAmount > maxUint64) {
      throw new Error(`Amount exceeds the maximum allowed value for uint64`);
    }
    if (account.vestingStart > maxUint32 || account.vestingEnd > maxUint32) {
      throw new Error(`Vesting exceeds the maximum allowed value for uint32`);
    }
    buff.writeBigUInt64LE(account.totalAmount, 0)
    buff.writeBigUInt64LE(account.initialUnlockAmount, 8)
    buff.writeUInt32LE(account.vestingStart, 16)
    buff.writeUInt32LE(account.vestingEnd, 20)
    const internalIndexBuffer = Buffer.from([internalIndex])
    const serializedAccount = Buffer.concat([buff, internalIndexBuffer, serializedOwnerAccount]);

    console.log(`Serialized account: ${serializedAccount.toString('hex')}`)

    return serializedAccount
  }

  private serializeAccount(account: Account): Buffer {
    // calc buffer len
    let buffLen: number = 0
    for (const pubkey of account.pubkeys) {
      // Each pubkey has to be stored in 33 bytes: 32 for pubkey and 1 for index
      buffLen += pubkey.pubkey.length + 1
    }

    // add 2 bytes to store approvers and participants
    let buff = Buffer.alloc(buffLen + 2)

    if (account.approvers > maxUint8 || account.participants > maxUint8) {
      throw new Error(`Approvers or participants exceed the maximum allowed value for uint8`);
    }
    buff.writeUInt8(account.approvers, 0)
    buff.writeUInt8(account.participants, 1)

    let indexOffset: number = 2;
    for (const pubkey of account.pubkeys) {
      buff.writeUInt8(pubkey.index, indexOffset)
      pubkey.pubkey.copy(buff, indexOffset + 1)
      indexOffset += pubkey.pubkey.length + 1
    }

    return buff
  }
}
