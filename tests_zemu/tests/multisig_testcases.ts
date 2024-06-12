import { addressToBuffer, PATH, PATH_TESTNET, pubKey0, pubKey2, pubKey3 } from './common'
import { AccountType } from '@zondax/ledger-spacemesh/src/types'

export const MULTISIG_TESTCASES = [
  {
    idx: 0,
    account: {
      pubkeys: [addressToBuffer(pubKey0, 0)],
      approvers: 2,
      participants: 2,
      id: AccountType.Multisig,
    },
    expected_address: 'sm1qqqqqq9yec9x0q84s8eqvsz9z82cfft0el4w2psknxxfl',
    expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    idx: 1,
    account: {
      pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3), addressToBuffer(pubKey2, 2)],
      approvers: 2,
      participants: 4,
      id: AccountType.Multisig,
    },
    expected_address: 'sm1qqqqqq8wmne37awzvphppdhms9564g9f73rel3c7cvxkl',
    expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    idx: 2,
    account: {
      pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3), addressToBuffer(pubKey2, 2)],
      approvers: 3,
      participants: 4,
      id: AccountType.Multisig,
    },
    expected_address: 'stest1qqqqqq9f7wcn6nm923pm26jar3vnwqdup0exvwsnypstf',
    expected_pk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
    path: PATH_TESTNET,
  },
]
