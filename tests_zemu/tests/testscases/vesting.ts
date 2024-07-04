import { addressToBuffer, pubKey0, pubKey2, pubKey3 } from './wallet'
import { PATH, PATH_TESTNET } from '../common'
import { TestCase } from './types'
import { Account, AccountType } from '@zondax/ledger-spacemesh'

export const VESTING_TESTCASES: TestCase[] = [
  {
    idx: 0,
    account: new Account(AccountType.Vesting, 2, 2, [addressToBuffer(pubKey0, 0)]),
    expected_address: 'sm1qqqqqq8yewvs5p0hnwfpw8q92j00pw8fq37g8yqr7x4vy',
    expected_pk: 'b7ec1a92bf5fd19cff888c2b7a278ceb6d649f5b678b89fb5c29bb1546f4a594',
    path: PATH,
  },
  {
    idx: 1,
    account: new Account(AccountType.Vesting, 2, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey3, 3),
      addressToBuffer(pubKey2, 2),
    ]),
    expected_address: 'sm1qqqqqqplv8uh93hqvfudg09g3khz9l7yhwwlu5qzqgxl6',
    expected_pk: 'b7ec1a92bf5fd19cff888c2b7a278ceb6d649f5b678b89fb5c29bb1546f4a594',
    path: PATH,
  },
  {
    idx: 2,
    account: new Account(AccountType.Vesting, 3, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey3, 3),
      addressToBuffer(pubKey2, 2),
    ]),
    expected_address: 'stest1qqqqqqyxkc2jdh6sg066xjh0xkaed8w4zlrsehqku8enj',
    expected_pk: '9cfbee82a799b8497430e8665f23e7b8e5b9345e6bbc6ee3895daa2ab8c00e16',
    path: PATH_TESTNET,
  },
]

export const VESTING_TRANSACTIONS = [
  {
    idx: 0,
    name: "vesting_spawn_0",
    blob: '9EEBFF023ABB17CCB775C602DAADE8ED708F0A5000000000001530AA316FECAAE1F94696EDD3D734DCCFC83F5A0000000000000000000000000000000000000000000000000300E50914145E23EA7417DF1425A60AFB6C91AE6D55A4D8DBD655B9A91C30B1E5A7DBD6CC1535EAC3A45AB5D617AD4AE7D26BBCE91C3CDDDFCA57713DD94C7B05E26422094D098C6869142426CDBAF5EEA80764565BBB67772397DC42BBD90E076F5033803E0AF42499826E951C611596E53747A1446D72FBBC0A08E421C94EF157EAEAD10898B481E481D9B07D25ADF9BD35F391DA3F6A223A8C7439179450A29928C521F3',
    path: PATH,
  },
  {
    idx: 1,
    name: "vesting_spend_0",
    blob: '9EEBFF023ABB17CCB775C602DAADE8ED708F0A5000000000001530AA316FECAAE1F94696EDD3D734DCCFC83F5A4000E50900000000719D58B3F3A1C73BECC9140B07A474DE6F5275EF0F7D2C8EDD863F59',
    path: PATH,
  },
]
