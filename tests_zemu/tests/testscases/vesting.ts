import { AccountType, Account } from '@zondax/ledger-spacemesh/src/types'
import { addressToBuffer, pubKey0, pubKey2, pubKey3 } from './wallet'
import { PATH, PATH_TESTNET } from '../common'
import { NoCheckAccount, TestCase } from './types'

export const VESTING_TESTCASES: TestCase[] = [
  {
    idx: 0,
    account: new Account(AccountType.Vesting, 2, 2, [addressToBuffer(pubKey0, 0)]),
    expected_address: 'sm1qqqqqq8987egzymqzujln6hpvd649zhf4p5mkdclnwtv0',
    expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    idx: 1,
    account: new Account(AccountType.Vesting, 2, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey3, 3),
      addressToBuffer(pubKey2, 2),
    ]),
    expected_address: 'sm1qqqqqq894p92556zmh22f8ywd2ehx0n06qj36pq47u9ey',
    expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    idx: 2,
    account: new Account(AccountType.Vesting, 3, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey3, 3),
      addressToBuffer(pubKey2, 2),
    ]),
    expected_address: 'stest1qqqqqq9d46ftwap48q3ydg2w58l5fmy4teh7l5gcmumn0',
    expected_pk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
    path: PATH_TESTNET,
  },
]
