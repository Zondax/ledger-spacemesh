import { addressToBuffer, PATH, PATH_TESTNET, pubKey0, pubKey2, pubKey3 } from './common'
import { AccountType } from '@zondax/ledger-spacemesh/src/types'

export const VAULT_TESTCASES = [
  {
    vaultAccount: {
      owner: {
        pubkeys: [addressToBuffer(pubKey0, 0)],
        approvers: 2,
        participants: 2,
        id: AccountType.Vesting,
      },
      totalAmount: BigInt(1000),
      initialUnlockAmount: BigInt(987),
      vestingStart: 567,
      vestingEnd: 99999,
      id: AccountType.Vault,
    },
    expected_address: 'sm1qqqqqqqrsssxatqkcxflze4eh8c4rngxsmudk7qqapryj',
    expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    vaultAccount: {
      owner: {
        pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3), addressToBuffer(pubKey2, 2)],
        approvers: 2,
        participants: 4,
        id: AccountType.Vesting,
      },
      totalAmount: BigInt(1000),
      initialUnlockAmount: BigInt(987),
      vestingStart: 567,
      vestingEnd: 44,
      id: AccountType.Vault,
    },
    expected_address: 'sm1qqqqqqz26rdxe5szj29fjafxm0yyg2mhazq96kg2kds04',
    expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
    path: PATH,
  },
  {
    vaultAccount: {
      owner: {
        pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3), addressToBuffer(pubKey2, 2)],
        approvers: 3,
        participants: 4,
        id: AccountType.Vesting,
      },
      totalAmount: BigInt(1000),
      initialUnlockAmount: BigInt(9876543210),
      vestingStart: 567,
      vestingEnd: 99999,
      id: AccountType.Vault,
    },
    expected_address: 'stest1qqqqqqpfj2kuy66r8zvkfsdg9xxvy0truqen8lg90z5av',
    expected_pk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
    path: PATH_TESTNET,
  },
]
