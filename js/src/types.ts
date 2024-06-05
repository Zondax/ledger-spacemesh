import { INSGeneric } from "@zondax/ledger-js";

export interface SpaceMeshIns extends INSGeneric {
  GET_VERSION: 0x00;
  GET_ADDR: 0x01;
  SIGN: 0x02;
}

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
  id: AccountType;
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