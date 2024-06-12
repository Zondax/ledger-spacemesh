import { INSGeneric } from "@zondax/ledger-js";

export interface ResponseAddress {
  pubkey: Buffer;
  address: string;
}

export interface ResponseSign {
  signature: Buffer;
}

export interface PubkeyItem {
  index: number;
  pubkey: Buffer;
}

export enum AccountType {
  Wallet = 1,
  Multisig = 2,
  Vesting = 3,
  Vault = 4
}

export interface Account {
  type: AccountType;
  participants: number;
  approvers: number;
  pubkeys: PubkeyItem[];
}

export interface VaultAccount extends Account {
  totalAmount: bigint;
  initialUnlockAmount: bigint;
  vestingStart: number;
  vestingEnd: number;
  type: AccountType.Vault;    // This account type is always fixed to AccountType.Vault
}

export enum Domain {
	ATX = 0,
	PROPOSAL = 1,
	BALLOT   = 2,
	HARE     = 3,
	POET     = 4,
	BEACON_FIRST_MSG    = 10,
	BEACON_FOLLOWUP_MSG = 11
}

// FIXME: document this
export interface EdSigner {
  prefix: Buffer;
  message: Buffer;
  domain: Domain;
}