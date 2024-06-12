import { IDeviceModel, DEFAULT_START_OPTIONS } from '@zondax/zemu'

import { resolve } from 'path'
import { Pubkey } from '@zondax/ledger-spacemesh/src/types'

export const APP_SEED = 'equip will roof matter pink blind book anxiety banner elbow sun young'

const APP_PATH_S = resolve('../app/output/app_s.elf')
const APP_PATH_X = resolve('../app/output/app_x.elf')
const APP_PATH_SP = resolve('../app/output/app_s2.elf')
const APP_PATH_ST = resolve('../app/output/app_stax.elf')

export const models: IDeviceModel[] = [
  //{ name: 'nanos', prefix: 'S', path: APP_PATH_S },
  //{ name: 'nanox', prefix: 'X', path: APP_PATH_X },
  //{ name: 'nanosp', prefix: 'SP', path: APP_PATH_SP },
  { name: 'stax', prefix: 'ST', path: APP_PATH_ST },
]

export const PATH = "m/44'/540'/0'/0/0"
export const PATH_TESTNET = "m/44'/1'/0'/0/0"

export const defaultOptions = {
  ...DEFAULT_START_OPTIONS,
  logging: true,
  custom: `-s "${APP_SEED}"`,
  X11: false,
}

export function addressToBuffer(stringPubKey: string, index: number): Pubkey {
  const bufferPubKey = Buffer.from(stringPubKey, 'hex')
  return { index, pubkey: bufferPubKey }
}

export const pubKey0 = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f8'
export const pubKey2 = '8ed90420802c83b41e4a7fa94ce5f05792ea8bff3d7a63572e5c73454eaef51d'
export const pubKey3 = '11bba3ed1721948cefb4e50b0a0bb5cad8a6b52dc7b1a40f4f6652105c91e2c4'

export const WALLET_TESTCASES = [
  {
    expectedAddress: 'sm1qqqqqqyjdl0e9t6s3cxx3pqgtmxh998w5l74v5syjt6w5',
    expectedPk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    expectedAddress: 'stest1qqqqqqr9l92twnvqkjgrmp9p086ekmdzgs79x5q0msfmy',
    expectedPk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
    path: PATH_TESTNET,
  },
]
