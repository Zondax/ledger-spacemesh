import { ByteStream } from '@zondax/ledger-js/dist/byteStream'

import { maxUint32, maxUint64 } from './consts'

export interface ResponseAddress {
  pubkey: Buffer
  address: string
}

export interface ResponseSign {
  signature: Buffer
}

export interface PubkeyItem {
  index: number
  pubkey: Buffer
}

export enum Domain {
  ATX = 0,
  PROPOSAL = 1,
  BALLOT = 2,
  HARE = 3,
  POET = 4,
  BEACON_FIRST_MSG = 10,
  BEACON_FOLLOWUP_MSG = 11,
}

// FIXME: document this
export interface EdSigner {
  prefix: Buffer
  message: Buffer
  domain: Domain
}

export enum AccountType {
  Wallet = 1,
  Multisig = 2,
  Vesting = 3,
  Vault = 4,
}

export interface AccountInterface {
  type: AccountType
  checkSanity(internalIndex: number): void
  serialize(): Buffer
}

export class Account implements AccountInterface {
  type: AccountType
  approvers: number
  participants: number
  pubkeys: PubkeyItem[]

  constructor(type: AccountType, approvers: number, participants: number, pubkeys: PubkeyItem[]) {
    this.type = type
    this.participants = participants
    this.approvers = approvers
    this.pubkeys = pubkeys
  }

  checkSanity(internalIndex: number) {
    const indices = new Set<number>()

    indices.add(internalIndex)
    for (const pubkey of this.pubkeys) {
      if (indices.has(pubkey.index)) {
        throw new Error(`Duplicate index ${pubkey.index} found in pubkeys array`)
      }
      indices.add(pubkey.index)

      if (pubkey.pubkey.length !== 32) {
        throw new Error(`Invalid pubkey size for ${pubkey.pubkey}`)
      }
    }

    for (let i = 0; i < indices.size; i++) {
      if (!indices.has(i)) {
        throw new Error(`Missing index ${i} in pubkeys array`)
      }
    }

    if (indices.size !== this.participants) {
      throw new Error(`Pubkey quantity does not match the number of participants`)
    }

    if (this.approvers === 0) {
      throw new Error(`Approvers cannot be 0`)
    }

    if (this.participants < this.approvers) {
      throw new Error(`Approvers cannot exceed the number of participants`)
    }

    this.pubkeys.sort((a, b) => a.index - b.index)
  }

  serialize() {
    const bs = new ByteStream()

    // FIXME: would it be possible to serialize this.type as a single byte?
    // that way we genealize the account concept?

    bs.appendUint8(this.approvers)
    bs.appendUint8(this.participants)
    for (const pubkey of this.pubkeys) {
      bs.appendUint8(pubkey.index)
      bs.appendBytes(pubkey.pubkey)
    }

    return bs.getCompleteBuffer()
  }
}

export class VaultAccount extends Account {
  totalAmount: bigint
  initialUnlockAmount: bigint
  vestingStart: number
  vestingEnd: number

  constructor(
    approvers: number,
    participants: number,
    pubkeys: PubkeyItem[],
    totalAmount: bigint,
    initialUnlockAmount: bigint,
    vestingStart: number,
    vestingEnd: number
  ) {
    super(AccountType.Vault, approvers, participants, pubkeys)
    this.totalAmount = totalAmount
    this.initialUnlockAmount = initialUnlockAmount
    this.vestingStart = vestingStart
    this.vestingEnd = vestingEnd
  }

  checkSanity(internalIndex: number): void {
    super.checkSanity(internalIndex)

    if (this.totalAmount > maxUint64 || this.initialUnlockAmount > maxUint64) {
      throw new Error(`Amount exceeds the maximum allowed value for uint64`)
    }
    if (this.vestingStart > maxUint32 || this.vestingEnd > maxUint32) {
      throw new Error(`Vesting exceeds the maximum allowed value for uint32`)
    }

    if (this.initialUnlockAmount > this.totalAmount) {
      throw new Error(`Total amount cannot be less than initial unlock amount`)
    }

    if (this.vestingStart > this.vestingEnd) {
      throw new Error(`Vesting start cannot be greater than vesting end`)
    }
  }

  serialize() {
    const bs = new ByteStream()

    bs.appendUint64(this.totalAmount)
    bs.appendUint64(this.initialUnlockAmount)
    bs.appendUint32(this.vestingStart)
    bs.appendUint32(this.vestingEnd)

    // add account data here
    bs.appendBytes(super.serialize())

    return bs.getCompleteBuffer()
  }
}
