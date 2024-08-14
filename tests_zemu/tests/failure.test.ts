/** ******************************************************************************
 *  (c) 2018 - 2024 Zondax AG
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************* */

import Zemu from '@zondax/zemu'
import { AccountType, PubkeyItem, SpaceMeshApp, VaultAccount } from '@zondax/ledger-spacemesh'
import { PATH, defaultOptions, models } from './common'
import { ResponseError } from '@zondax/ledger-js'
import { addressToBuffer } from './testscases/wallet'
import { NoCheckAccount, NoCheckVaultAccount } from './testscases/types'

jest.setTimeout(100000)

describe('Failure scenarios', function () {
  test('do nothing', function () {})
})

const pubKey0 = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f8'
const pubKey2 = '8ed90420802c83b41e4a7fa94ce5f05792ea8bff3d7a63572e5c73454eaef51d'
const pubKey3 = '11bba3ed1721948cefb4e50b0a0bb5cad8a6b52dc7b1a40f4f6652105c91e2c4'
const pubKeyWrong = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f'

const testCases1 = [
  {
    scenario: 'Duplicate index in pubkeys array',
    owner: new NoCheckAccount(AccountType.Vesting, 2, 2, [addressToBuffer(pubKey0, 1)]),
  },
  {
    scenario: 'Unsorted array',
    owner: new NoCheckAccount(AccountType.Vesting, 2, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey2, 2),
      addressToBuffer(pubKey3, 4),
    ]),
  },
  {
    scenario: 'One index is equal to or greater than participants',
    owner: new NoCheckAccount(AccountType.Vesting, 2, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey2, 2),
      addressToBuffer(pubKey3, 5),
    ]),
  },
  {
    scenario: 'First pubkey is missing',
    vaultAccount: {
      owner: {
        pubkeys: [addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
        approvers: 2,
        participants: 4,
        id: AccountType.Vesting,
      },
    },
  },
  {
    scenario: 'Last pubkey is missing',
    owner: new NoCheckAccount(AccountType.Vesting, 2, 4, [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2)]),
  },
  {
    scenario: 'A pubkey is missing from the middle of the sequence',
    owner: new NoCheckAccount(AccountType.Vesting, 2, 4, [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3)]),
  },
  {
    scenario: 'participants < approvers',
    owner: new NoCheckAccount(AccountType.Vesting, 5, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey2, 2),
      addressToBuffer(pubKey3, 3),
    ]),
  },
  {
    scenario: 'approvers == 0',
    owner: new NoCheckAccount(AccountType.Vesting, 0, 4, [
      addressToBuffer(pubKey0, 0),
      addressToBuffer(pubKey2, 2),
      addressToBuffer(pubKey3, 3),
    ]),
  },
  {
    scenario: 'Corrupted pubkey',
    owner: new NoCheckAccount(AccountType.Vesting, 2, 4, [
      addressToBuffer(pubKeyWrong, 0),
      addressToBuffer(pubKey2, 2),
      addressToBuffer(pubKey3, 3),
    ]),
  },
]

const testCases100 = [
  {
    scenario: 'Duplicate index in pubkeys array',
    owner: {
      pubkeys: [addressToBuffer(pubKey0, 1)],
      approvers: 2,
      participants: 2,
      id: AccountType.Vesting,
    },
    expectedError: 'Duplicate index 1 found in pubkeys array',
  },
  {
    scenario: 'One index is equal to or greater than participants',
    owner: {
      pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 4), addressToBuffer(pubKey2, 2)],
      approvers: 2,
      participants: 4,
      id: AccountType.Vesting,
    },
    expectedError: 'Missing index 3 in pubkeys array',
  },
  {
    scenario: 'First pubkey is missing',
    vaultAccount: {
      owner: {
        pubkeys: [addressToBuffer(pubKey3, 3), addressToBuffer(pubKey2, 2)],
        approvers: 2,
        participants: 4,
        id: AccountType.Vesting,
      },
    },
    expectedError: 'Missing index 0 in pubkeys array',
  },
  {
    scenario: 'Last pubkey is missing',
    owner: {
      pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2)],
      approvers: 2,
      participants: 4,
      id: AccountType.Vesting,
    },
    expectedError: 'Pubkey quantity does not match the number of participants',
  },
  {
    scenario: 'A pubkey is missing from the middle of the sequence',
    owner: {
      pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3)],
      approvers: 2,
      participants: 4,
      id: AccountType.Vesting,
    },
    expectedError: 'Missing index 2 in pubkeys array',
  },
  {
    scenario: 'participants < approvers',
    owner: {
      pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
      approvers: 5,
      participants: 4,
      id: AccountType.Vesting,
    },
    expectedError: 'Approvers cannot exceed the number of participants',
  },
  {
    scenario: 'approvers == 0',
    owner: {
      pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
      approvers: 0,
      participants: 4,
      id: AccountType.Vesting,
    },
    expectedError: 'Approvers cannot be 0',
  },
]

const testCases200 = [
  {
    scenario: 'TotalAmount overflow',
    totalAmount: BigInt('0x10000000000000000'),
    initialUnlockAmount: BigInt(987),
    vestingStart: 567,
    vestingEnd: 99999,
    expectedError: 'Amount exceeds the maximum allowed value for uint64',
  },
  {
    scenario: 'TotalAmount overflow',
    totalAmount: BigInt(1000),
    initialUnlockAmount: BigInt('0x10000000000000000'),
    vestingStart: 567,
    vestingEnd: 99999,
    expectedError: 'Amount exceeds the maximum allowed value for uint64',
  },
  {
    scenario: 'VestingStart overflow',
    totalAmount: BigInt(1000),
    initialUnlockAmount: BigInt(987),
    vestingStart: 0x100000000,
    vestingEnd: 99999,
    expectedError: 'Vesting exceeds the maximum allowed value for uint32',
  },
  {
    scenario: 'VestingEnd overflow',
    totalAmount: BigInt(1000),
    initialUnlockAmount: BigInt(987),
    vestingStart: 567,
    vestingEnd: 0x100000000,
    expectedError: 'Vesting exceeds the maximum allowed value for uint32',
  },
]

describe('Failure scenarios', function () {
  test.each(models)('can start and stop container', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
    } finally {
      await sim.close()
    }
  })

  test.each(models)('Invalid data in api', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      for (const testCase of testCases1) {
        const owner = testCase.owner ? testCase.owner : { pubkeys: [], participants: 0, approvers: 0, id: AccountType.Vesting }
        const vaultAccount = new NoCheckVaultAccount(
          owner.approvers,
          owner.participants,
          owner.pubkeys,
          BigInt(1000),
          BigInt(987),
          567,
          99999,
        )

        console.log('testing: ', testCase.scenario)
        await expect(app.getAddressVault(PATH, 1, vaultAccount)).rejects.toThrow(
          new ResponseError(0x6984, 'Data is invalid : Invalid crypto settings'),
        )
      }
    } finally {
      await sim.close()
    }
  })

  test.each(models)('invalid data in js', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      for (const test of testCases100) {
        const owner = test.owner ? test.owner : { pubkeys: [], participants: 0, approvers: 0, id: AccountType.Vesting }
        const vaultAccount = new VaultAccount(owner.approvers, owner.participants, owner.pubkeys, BigInt(1000), BigInt(987), 567, 99999)

        console.log('Testing: ', test.scenario)
        await expect(app.getAddressVault(PATH, 1, vaultAccount)).rejects.toThrow(new Error(test.expectedError))
      }

      for (const test of testCases200) {
        const vaultAccount = new VaultAccount(
          2,
          4,
          [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
          test.totalAmount,
          test.initialUnlockAmount,
          test.vestingStart,
          test.vestingEnd,
        )

        console.log('Testing: ', test.scenario)
        await expect(app.getAddressVault(PATH, 1, vaultAccount)).rejects.toThrow(new Error(test.expectedError))
      }
    } finally {
      await sim.close()
    }
  })

  function addressToBuffer(stringPubKey: string, index: number): PubkeyItem {
    const bufferPubKey = Buffer.from(stringPubKey, 'hex')
    return { index, pubkey: bufferPubKey }
  }
})

describe('Failure scenarios', function () {
  test.each(models)('can start and stop container', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
    } finally {
      await sim.close()
    }
  })

  test.each(models)('Invalid data in api', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      for (const testCase of testCases1) {
        const owner = testCase.owner ? testCase.owner : { pubkeys: [], participants: 0, approvers: 0, id: AccountType.Vesting }
        const vaultAccount = new NoCheckVaultAccount(
          owner.approvers,
          owner.participants,
          owner.pubkeys,
          BigInt(1000),
          BigInt(987),
          567,
          99999,
        )

        console.log('testing: ', testCase.scenario)
        await expect(app.getAddressVault(PATH, 1, vaultAccount)).rejects.toThrow(
          new ResponseError(0x6984, 'Data is invalid : Invalid crypto settings'),
        )
      }
    } finally {
      await sim.close()
    }
  })

  test.each(models)('invalid data in js', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      for (const test of testCases100) {
        const owner = test.owner ? test.owner : { pubkeys: [], participants: 0, approvers: 0, id: AccountType.Vesting }
        const vaultAccount = new VaultAccount(owner.approvers, owner.participants, owner.pubkeys, BigInt(1000), BigInt(987), 567, 99999)

        console.log('Testing: ', test.scenario)
        await expect(app.getAddressVault(PATH, 1, vaultAccount)).rejects.toThrow(new Error(test.expectedError))
      }

      for (const test of testCases200) {
        const vaultAccount = new VaultAccount(
          2,
          4,
          [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
          test.totalAmount,
          test.initialUnlockAmount,
          test.vestingStart,
          test.vestingEnd,
        )

        console.log('Testing: ', test.scenario)
        await expect(app.getAddressVault(PATH, 1, vaultAccount)).rejects.toThrow(new Error(test.expectedError))
      }
    } finally {
      await sim.close()
    }
  })

  function addressToBuffer(stringPubKey: string, index: number): PubkeyItem {
    const bufferPubKey = Buffer.from(stringPubKey, 'hex')
    return { index, pubkey: bufferPubKey }
  }
})
