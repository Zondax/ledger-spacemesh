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
    expectedAddress: 'sm1qqqqqqp6qjvvlg7h9z748an72kcx3sjqyjcvrkg0jfst2',
    expectedPk: 'b7ec1a92bf5fd19cff888c2b7a278ceb6d649f5b678b89fb5c29bb1546f4a594',
    path: PATH,
  },
  {
    expectedAddress: 'stest1qqqqqqyt8ps0ze9r7zevlqej3ddu2d8gxl6sp5g8vput3',
    expectedPk: '9cfbee82a799b8497430e8665f23e7b8e5b9345e6bbc6ee3895daa2ab8c00e16',
    path: PATH_TESTNET,
  },
]

export const WALLET_TRANSACTIONS = [
  {
    idx: 0,
    name: "wallet_spawn_0",
    blob: '0000000000bde1127a3898a73ef9a0c963f9a62f06e14d83350000000000000000000000000000000000000000000000000100046f6866a1b8317d47fb8adfee9709322a18be9afad837006b5369d724fcdd4c09',
    path: PATH,
  },
  {
    idx: 1,
    name: "wallet_spend_0",
    blob: '00000000003f7f865bfd4d3b29c2cd37d6aecefb72a6dbc574400404000000009eba337d5871329b6cb89d73f6b948cecd8b712a9101',
    path: PATH,
  },
]
