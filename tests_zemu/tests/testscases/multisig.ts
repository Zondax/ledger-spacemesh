import { AccountType, Account } from '@zondax/ledger-spacemesh'
import { addressToBuffer, pubKey0, pubKey2, pubKey3 } from './wallet'
import { PATH, PATH_TESTNET } from '../common'
import { TestCase } from './types'

export const MULTISIG_TESTCASES: TestCase[] = [
  {
    idx: 0,
    account: new Account(AccountType.Multisig, 2, 2, [addressToBuffer(pubKey0, 0)]),
    expected_address: 'sm1qqqqqq9yec9x0q84s8eqvsz9z82cfft0el4w2psknxxfl',
    expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    idx: 1,
    account: new Account(AccountType.Multisig, 2, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey3, 3),
      addressToBuffer(pubKey2, 2),
    ]),
    expected_address: 'sm1qqqqqq8wmne37awzvphppdhms9564g9f73rel3c7cvxkl',
    expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    idx: 2,
    account: new Account(AccountType.Multisig, 3, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey3, 3),
      addressToBuffer(pubKey2, 2),
    ]),
    expected_address: 'stest1qqqqqq9f7wcn6nm923pm26jar3vnwqdup0exvwsnypstf',
    expected_pk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
    path: PATH_TESTNET,
  },
]

export const MULTISIG_TRANSACTIONS = [
  {
    idx: 0,
    name: "multisig_spawn_0",
    blob: '0000000000b1885c09432cb9de46fd1997f452a53843ab1cc70000000000000000000000000000000000000000000000000200040c14218506d35db6a7bab8d3717d55ade4c43ca80f7291883cd549ff8a34b78324bd00b9a50aadc89a95a2a14eb7594664d9910e85b5d6f5abbc5788223d4c16692101fa619348966aaba9c94b22d43c3703e73bd54fa6452112e15febc5b9a6ab1a4242fc8d73581ae4da1f2ec206db38fd1605d7781ce9fb647f4b3049ac8f309dcecf7ee7dda78952c3782c334864ad7c9458e22e0e16733db1d2bc7c8bc9ef61',
    path: PATH,
  },
  {
    idx: 1,
    name: "multisig_spend_0",
    blob: '0000000000b1885c09432cb9de46fd1997f452a53843ab1cc740040400000000b1885c09432cb9de46fd1997f452a53843ab1cc728',
    path: PATH,
  },
]
