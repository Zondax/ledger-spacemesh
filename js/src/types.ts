import { INSGeneric } from "@zondax/ledger-js";

export interface ResponseAddress {
  pubkey: Buffer;
  address: string;
}

export interface ResponseSign {
  signature: Buffer;
}

export interface Pubkey {
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
  pubkeys: Pubkey[];
  participants: number;
  approvers: number;
  id: AccountType;
}

export interface VaultAccount {
  owner: Account;
  totalAmount: bigint;
  initialUnlockAmount: bigint;
  vestingStart: number;
  vestingEnd: number;
  id: AccountType;          // FIXME: why is this here? should be always fixed? AccountType.Vault
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
export interface EdSigner {
  prefix: Buffer;
  message: Buffer;
  domain: Domain;
}