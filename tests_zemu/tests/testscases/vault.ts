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
    genesisId: '9eebff023abb17ccb775c602daade8ed708f0a50',
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
    genesisId: '9eebff023abb17ccb775c602daade8ed708f0a50',
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
    genesisId: 'e956eff99be943fb70bd385dd509a3f84e9a75dd',
  },
]

export const VAULT_TRANSACTIONS = [
  {
    idx: 0,
    name: "vault_spawn_0",
    blob: '9EEBFF023ABB17CCB775C602DAADE8ED708F0A50000000000042DE3E036234EA49821410097A39DAE01B0AEC310000000000000000000000000000000000000000000000000300B10F141CD6233F55A5303EB324E94EC1E02E93F7CC70255E5803018A07C9E29F10075757A603A959D883727AE567E98047DEADC6E922586D75557F49A990A6418BAD31542AF6B7AE9A256FE3936A515F45A6A52C188C61145FDE597F07F628B7192B070AF29EE9A4F232B60C0084DFFF2C2358568C51681A76F3179BE6E465145772D3CFEA81ADDB0216354EF0F9C4494B23842EE97951E01470F184C3B18730D6EBF573A6DDDD0E98B53582E875B06B6B1198197013D325A3248D352EC423C7C5F3DCD0EDFDBE2545C6950614DA0F7A97D4625B1D6182E2643333E6D00340D1E9067B41',
    path: PATH,
  },
  {
    idx: 2,
    name: "vault_drain_0",
    blob: '9EEBFF023ABB17CCB775C602DAADE8ED708F0A50000000000042DE3E036234EA49821410097A39DAE01B0AEC314400B10F00000000F6628EFA0993A5B9BC11AEB18C01A61E00307D3700000000719D58B3F3A1C73BECC9140B07A474DE6F5275EF0FC7662A51678293',
    path: PATH,
  }
]