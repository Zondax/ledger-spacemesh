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

import { Account, ResponseAddress, AccountType } from "./types";
import { P1_VALUES, PUBKEYLEN } from './consts'

import { ResponseSign } from './types'

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

  async getAddressAndPubKeyMultisig(path: string, internalIndex: number, account: Account): Promise<ResponseAddress>{
    const accountsOrdered = checkAccountsSanity(internalIndex, account);
    const serializedAccount = serializeAccount(accountsOrdered);
    const payload = Buffer.concat([Buffer.from([internalIndex]), serializedAccount]);

    const chunks = this.prepareChunks(path, payload);

    try {
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

  async sign(path: BIP32Path, blob: Buffer): Promise<ResponseSign> {
    const chunks = this.prepareChunks(path, blob);
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
  
  getInstruction(id: AccountType): number {
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
}


function checkAccountsSanity(internalIndex: number, account: Account): Account {
  const indices = new Set<number>()
  indices.add(internalIndex)
  for (const pubkey of account.pubkeys) {
    if (indices.has(pubkey.index)) {
      throw new Error(`Duplicate index ${pubkey.index} found in pubkeys array`)
    }
    indices.add(pubkey.index)
  }
  
  for (let i = 0; i < account.pubkeys.length; i++) {
    if (!indices.has(i)) {
      throw new Error(`Missing index ${i} in pubkeys array`)
    }
  }
  
  return account
}

function serializeAccount(account: Account): Buffer {
  // TODO: make 33 a defined const
  const sizePubkey: number = 33
  let buff = Buffer.alloc(sizePubkey*account.pubkeys.length + 2)

  buff.writeUInt8(account.approvers, 0)
  buff.writeUInt8(account.participants, 1)

  let indexOffset: number = 2;
  for (const pubkey of account.pubkeys) {
    buff.writeUInt8(pubkey.index, indexOffset);
    pubkey.pubkey.copy(buff, indexOffset + 1);
    console.log("serializeAccount", indexOffset, pubkey)
    indexOffset += sizePubkey;
  }

  return buff
}

