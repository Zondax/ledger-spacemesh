import { INSGeneric } from "@zondax/ledger-js";

export interface SpaceMeshIns extends INSGeneric {
  GET_VERSION: 0x00;
  GET_ADDR: 0x01;
  SIGN: 0x02;
  GET_MULTISIG_VESTING: 0x03;
  GET_ADDR_VAULT: 0x04;
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

export interface Account {
  pubkeys: Pubkey[];
  participants: number;
  approvers: number;
}
