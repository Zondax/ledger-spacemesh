/** ******************************************************************************
 *  (c) 2018 - 2023 Zondax AG
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
import { SpaceMeshApp } from '@zondax/ledger-spacemesh'
import { PATH, defaultOptions, models } from './common'
import { Pubkey, AccountType, EdSigner, Domain, VaultAccount } from '@zondax/ledger-spacemesh/src/types'
import { ResponseError } from '@zondax/ledger-js'

// @ts-expect-error
import ed25519 from 'ed25519-supercop'

jest.setTimeout(60000)

describe('Failure scenarios', function () {
  test.concurrent.each(models)('can start and stop container', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(models)('Invalid data in api', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())
      const pubKey0 = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f8'
      const pubKey2 = '8ed90420802c83b41e4a7fa94ce5f05792ea8bff3d7a63572e5c73454eaef51d'
      const pubKey3 = '11bba3ed1721948cefb4e50b0a0bb5cad8a6b52dc7b1a40f4f6652105c91e2c4'
      const pubKeyWrong = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f'

      const testCases = [
        {
          scenario: 'Duplicate index in pubkeys array',
          owner: {
            pubkeys: [addressToBuffer(pubKey0, 1)],
            approvers: 2,
            participants: 2,
            id: AccountType.Vesting,
          },
        },
        {
          scenario: 'Unsorted array',
          owner: {
            pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 4), addressToBuffer(pubKey2, 2)],
            approvers: 2,
            participants: 4,
            id: AccountType.Vesting,
          },
        },
        {
          scenario: 'One index is equal to or greater than participants',
          owner: {
            pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 4)],
            approvers: 2,
            participants: 4,
            id: AccountType.Vesting,
          },
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
          owner: {
            pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2)],
            approvers: 2,
            participants: 4,
            id: AccountType.Vesting,
          },
        },
        {
          scenario: 'A pubkey is missing from the middle of the sequence',
          owner: {
            pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey3, 3)],
            approvers: 2,
            participants: 4,
            id: AccountType.Vesting,
          },
        },
        {
          scenario: 'participants < approvers',
          owner: {
            pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
            approvers: 5,
            participants: 4,
            id: AccountType.Vesting,
          },
        },
        {
          scenario: 'approvers == 0',
          owner: {
            pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
            approvers: 0,
            participants: 4,
            id: AccountType.Vesting,
          },
        },
        {
          scenario: 'Corrupted pubkey',
          owner: {
            pubkeys: [addressToBuffer(pubKeyWrong, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
            approvers: 2,
            participants: 4,
            id: AccountType.Vesting,
          },
        },
      ]

      for (const testCase of testCases) {
        const owner = testCase.owner ? testCase.owner : { pubkeys: [], participants: 0, approvers: 0, id: AccountType.Vesting }
        const vaultAccount: VaultAccount = {
          owner,
          totalAmount: BigInt(1000),
          initialUnlockAmount: BigInt(987),
          vestingStart: 567,
          vestingEnd: 99999,
          id: AccountType.Vault,
        }
        console.log('testing: ', testCase.scenario)
        await expect(app.getInfoVaultAccount(PATH, 1, vaultAccount, true)).rejects.toThrow(
          new ResponseError(0x6984, 'Data is invalid : Invalid crypto settings'),
        )
      }
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(models)('invalid data in js', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      const pubKey0 = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f8'
      const pubKey2 = '8ed90420802c83b41e4a7fa94ce5f05792ea8bff3d7a63572e5c73454eaef51d'
      const pubKey3 = '11bba3ed1721948cefb4e50b0a0bb5cad8a6b52dc7b1a40f4f6652105c91e2c4'

      const testCases = [
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

      for (const test of testCases) {
        const owner = test.owner ? test.owner : { pubkeys: [], participants: 0, approvers: 0, id: AccountType.Vesting }
        const vaultAccount: VaultAccount = {
          owner,
          totalAmount: BigInt(1000),
          initialUnlockAmount: BigInt(987),
          vestingStart: 567,
          vestingEnd: 99999,
          id: AccountType.Vault,
        }
        console.log('Testing: ', test.scenario)
        await expect(app.getInfoVaultAccount(PATH, 1, vaultAccount, false)).rejects.toThrow(new Error(test.expectedError))
      }

      const testCases2 = [
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
      for (const test of testCases2) {
        const vaultAccount: VaultAccount = {
          owner: {
            pubkeys: [addressToBuffer(pubKey0, 0), addressToBuffer(pubKey2, 2), addressToBuffer(pubKey3, 3)],
            approvers: 2,
            participants: 4,
            id: AccountType.Vesting,
          },

          totalAmount: test.totalAmount,
          initialUnlockAmount: test.initialUnlockAmount,
          vestingStart: test.vestingStart,
          vestingEnd: test.vestingEnd,
          id: AccountType.Vault,
        }
        console.log('Testing: ', test.scenario)
        await expect(app.getInfoVaultAccount(PATH, 1, vaultAccount, false)).rejects.toThrow(new Error(test.expectedError))
      }
    } finally {
      await sim.close()
    }
  })

  function addressToBuffer(stringPubKey: string, index: number): Pubkey {
    const bufferPubKey = Buffer.from(stringPubKey, 'hex')
    return { index, pubkey: bufferPubKey }
  }

  // #{TODO} --> Add Zemu tests for different transactions. Include expert mode if needed
  test.concurrent.each(models)('sign blind', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      const responseAddr = await app.getAddressAndPubKey(PATH)
      const pubKey = responseAddr.pubkey

      const test = Buffer.from('This is a dummy message that will be signed by Spacemesh app')
      let singInfo: EdSigner = {
        prefix: Buffer.from('test'),
        domain: Domain.HARE,
        message: test,
      }

      // do not wait here.. we need to navigate
      const signatureRequest = app.sign(PATH, singInfo)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_blind`)

      const signatureResponse = await signatureRequest
      console.log(signatureResponse)

      // Now verify the signature
      const payload = Buffer.concat([singInfo.prefix, Buffer.from([singInfo.domain]), singInfo.message])
      const valid = ed25519.verify(signatureResponse.signature, payload, pubKey)
      expect(valid).toEqual(true)
    } finally {
      await sim.close()
    }
  })
})
