import { PubkeyItem } from '@zondax/ledger-spacemesh'
import { PATH, PATH_TESTNET } from '../common'

export function addressToBuffer(stringPubKey: string, index: number): PubkeyItem {
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
