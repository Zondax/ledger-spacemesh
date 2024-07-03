import { AccountType, Account } from '@zondax/ledger-spacemesh'
import { addressToBuffer, pubKey0, pubKey2, pubKey3 } from './wallet'
import { PATH, PATH_TESTNET } from '../common'
import { TestCase } from './types'

export const MULTISIG_TESTCASES: TestCase[] = [
  {
    idx: 0,
    account: new Account(AccountType.Multisig, 2, 2, [addressToBuffer(pubKey0, 0)]),
    expected_address: 'sm1qqqqqqyyms4sgqlmeggm8pn0qhqr26j2eawzdmsa4ydz5',
    expected_pk: 'b7ec1a92bf5fd19cff888c2b7a278ceb6d649f5b678b89fb5c29bb1546f4a594',
    path: PATH,
  },
  {
    idx: 1,
    account: new Account(AccountType.Multisig, 2, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey3, 3),
      addressToBuffer(pubKey2, 2),
    ]),
    expected_address: 'sm1qqqqqq805rnfrv3d5hr8qy3s954cx7rfd66nc4gjv9hs0',
    expected_pk: 'b7ec1a92bf5fd19cff888c2b7a278ceb6d649f5b678b89fb5c29bb1546f4a594',
    path: PATH,
  },
  {
    idx: 2,
    account: new Account(AccountType.Multisig, 3, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey3, 3),
      addressToBuffer(pubKey2, 2),
    ]),
    expected_address: 'stest1qqqqqqpeuxeextghddk4jfmgt7g7ynqfshdclng399zkc',
    expected_pk: '9cfbee82a799b8497430e8665f23e7b8e5b9345e6bbc6ee3895daa2ab8c00e16',
    path: PATH_TESTNET,
  },
]

export const MULTISIG_TRANSACTIONS = [
  {
    idx: 0,
    name: "multisig_spawn_0",
    blob: '9EEBFF023ABB17CCB775C602DAADE8ED708F0A5000000000008E2FFE6A66BDBC0230331A778E10BEFB77A1B7E50000000000000000000000000000000000000000000000000200F12404104E9941A547FD184BB0F2EFA8495E1F9ECC1A2D4B32CE7794EC3533A2EB0B802FA8BF76F261D21BD799524AD54D0AA924ECD6622D48BB0BC5E20926F5E0FE2F68491ADA596A7A53FB6749DE21A2E75FFCF139A3C3B17170678B3278EDCEAA570DC6014D00283780A9E353ED60560EF520206053AAA37D7C50997A36A6AEC5A877',
    path: PATH,
  },
  {
    idx: 1,
    name: "multisig_spend_0",
    blob: '9EEBFF023ABB17CCB775C602DAADE8ED708F0A5000000000008E2FFE6A66BDBC0230331A778E10BEFB77A1B7E54000F12400000000719D58B3F3A1C73BECC9140B07A474DE6F5275EF13416B33AA0E225001',
    path: PATH,
  },
]
