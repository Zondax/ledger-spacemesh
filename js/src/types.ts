import { INSGeneric } from "@zondax/ledger-js";

export interface SpaceMeshIns extends INSGeneric {
  GET_VERSION: 0x00;
  GET_ADDR: 0x01;
  SIGN: 0x02;
  GET_MULTISIG_ADDR: 0x03;
  GET_VESTING_ADDR: 0x04;
  GET_VAULT_ADDR: 0x05;
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
