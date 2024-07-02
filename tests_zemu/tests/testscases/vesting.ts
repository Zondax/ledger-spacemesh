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
    blob: '0000000000df9d59520a4e7d96c473ac8a53814b6c9cb9222d0000000000000000000000000000000000000000000000000300040c28416f4ee52b75ea4c44249311a08e59d718687fdb3ed249fb85eb3e15a3e5666ef0f83b12faca1c4abd9684af8df1d501e982252697f7cf23c05f8e2e7de95cb4f9495d6dd982aec2acbf3c933c12d9fd223a8e3804396b6ae493501bdeca214599e87e98b9440ffec65e47c69a6d96e230f57e3c2b42fb571223b2b79a715e3d8e002de81e72ba6aa367fcf24bacfb56414bbf390526ccc222d0b4d3669ef4bb18320a5acd0e745be319b44b1eed1337f4a2ac03a568a5ddbd4326f0e0e0fff901580e263fced9409ec46abd89fd5aaf17a410d30911f8c935294466084f398c381d0355afee4d85036c0dca2850a43e5dee00bbe5c279c51544fc60afb50a110c065ef54dbe55c63f8d1b3f6eb7f01d45077784503fed35473f481c18b73118e9d6e35135d9ef5a949c688aa40969e65dea0d19377af36390e6ed1a9ff21e8e',
    path: PATH,
  },
  {
    idx: 1,
    name: "vesting_spend_0",
    blob: '0000000000df9d59520a4e7d96c473ac8a53814b6c9cb9222d400404000000001618dbc950ea0fc4970585209bf4fe6964020d739101',
    path: PATH,
  },
]
