import { PATH, PATH_TESTNET } from '../common'
import { addressToBuffer, pubKey0, pubKey2, pubKey3 } from './wallet'
import { TestCase } from './types'
import { VaultAccount } from '@zondax/ledger-spacemesh'

export const VAULT_TESTCASES: TestCase[] = [
  {
    idx: 0,
    account: new VaultAccount(2, 2, [addressToBuffer(pubKey0, 0)], BigInt(1000), BigInt(987), 567, 99999),
    expected_address: 'sm1qqqqqqy6gw3ufy6kzutpg03zjr0t9k8xjdagwuccmpd8g',
    expected_pk: 'b7ec1a92bf5fd19cff888c2b7a278ceb6d649f5b678b89fb5c29bb1546f4a594',
    path: PATH,
  },
  {
    idx: 1,
    account: new VaultAccount(
      3,
      4,
      [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3), addressToBuffer(pubKey2, 2)],
      BigInt(1000),
      BigInt(987),
      44,
      567,
    ),
    expected_address: 'sm1qqqqqq8d2nscr9hnvy8khzgxggg2vxg2jtplp0gqnklav',
    expected_pk: 'b7ec1a92bf5fd19cff888c2b7a278ceb6d649f5b678b89fb5c29bb1546f4a594',
    path: PATH,
  },
  {
    idx: 2,
    account: new VaultAccount(
      3,
      4,
      [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3), addressToBuffer(pubKey2, 2)],
      BigInt(9876543210),
      BigInt(1000),
      567,
      99999,
    ),
    expected_address: 'stest1qqqqqqx3pllnhamat3a4kmwexd39kkgst7p52jqjwx5wl',
    expected_pk: '9cfbee82a799b8497430e8665f23e7b8e5b9345e6bbc6ee3895daa2ab8c00e16',
    path: PATH_TESTNET,
  },
]

export const VAULT_TRANSACTIONS = [
  {
    idx: 0,
    name: "vault_spawn_0",
    blob: '00000000003161d16912af61f88a972ef9215acb93d36624f7000000000000000000000000000000000000000000000000040404000000003161d16912af61f88a972ef9215acb93d36624f79101280428',
    path: PATH,
  },
  {
    idx: 1,
    name: "vault_spawn_1",
    blob: '00000000001d68968022c78e97220f0ab85c63702883198e37000000000000000000000000000000000000000000000000040404000000001d68968022c78e97220f0ab85c63702883198e370f008053ee7ba80a0f0080c6a47e8d03826a060002aa1900',
    path: PATH,
  },
  {
    idx: 2,
    name: "vault_drain_0",
    blob: '00000000001d68968022c78e97220f0ab85c63702883198e3744080400000000fdd044582aef34274679ac50bc263ee0797933e0000000001d68968022c78e97220f0ab85c63702883198e3702093d00',
    path: PATH,
  }
]