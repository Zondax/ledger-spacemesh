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

import Zemu, { zondaxMainmenuNavigation } from '@zondax/zemu'
import { SpaceMeshApp } from '@zondax/ledger-spacemesh'
import { PATH, TESTNET, defaultOptions, models } from './common'
import { Pubkey, AccountType, EdSigner, Domain } from '../../js/src/types';

// @ts-expect-error
import ed25519 from 'ed25519-supercop'

jest.setTimeout(60000)

describe('Standard', function () {
  test.concurrent.each(models)('can start and stop container', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(models)('main menu', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const nav = zondaxMainmenuNavigation(m.name, [1, 0, 0, 4, -5])
      await sim.navigateAndCompareSnapshots('.', `${m.prefix.toLowerCase()}-mainmenu`, nav.schedule)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(models)('get app version', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())
      try {
        const resp = await app.getVersion()
        console.log(resp)

        expect(resp).toHaveProperty('testMode')
        expect(resp).toHaveProperty('major')
        expect(resp).toHaveProperty('minor')
        expect(resp).toHaveProperty('patch')
      } catch {
        console.log("getVersion error")
      }
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(models)('get address', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())
      const testCases = [
        {
          expectedAddress: 'sm1qqqqqqyjdl0e9t6s3cxx3pqgtmxh998w5l74v5syjt6w5',
          expectedPk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
          path: PATH,
        },
        {
          expectedAddress: 'stest1qqqqqqr9l92twnvqkjgrmp9p086ekmdzgs79x5q0msfmy',
          expectedPk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
          path: TESTNET,
        }
      ];

      for (const testCase of testCases) {
        const resp = await app.getAddressAndPubKey(testCase.path);
        console.log(resp);
        expect(resp.pubkey?.toString('hex')).toEqual(testCase.expectedPk);
        expect(resp.address).toEqual(testCase.expectedAddress);
      }

    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(models)('get multisig address', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      const pubKey0 = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f8';
      const pubKey2 = '8ed90420802c83b41e4a7fa94ce5f05792ea8bff3d7a63572e5c73454eaef51d';
      const pubKey3 = '11bba3ed1721948cefb4e50b0a0bb5cad8a6b52dc7b1a40f4f6652105c91e2c4';

      const testCases = [
        {
          account: {
            pubkeys: [addressToBuffer(pubKey0, 0)],
            approvers: 2,
            participants: 2,
            id: AccountType.Multisig
          },
          expected_address: 'sm1qqqqqq9yec9x0q84s8eqvsz9z82cfft0el4w2psknxxfl',
          expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
          path: PATH
        },
        {
          account: {
            pubkeys: [
              addressToBuffer(pubKey0, 0),
              addressToBuffer(pubKey3, 3),
              addressToBuffer(pubKey2, 2)
            ],
            approvers: 2,
            participants: 4,
            id: AccountType.Multisig
          },
          expected_address: 'sm1qqqqqq8wmne37awzvphppdhms9564g9f73rel3c7cvxkl',
          expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
          path: PATH
        },
        {
          account: {
            pubkeys: [
              addressToBuffer(pubKey0, 0),
              addressToBuffer(pubKey3, 3),
              addressToBuffer(pubKey2, 2)
            ],
            approvers: 3,
            participants: 4,
            id: AccountType.Multisig
          },
          expected_address: 'stest1qqqqqq9f7wcn6nm923pm26jar3vnwqdup0exvwsnypstf',
          expected_pk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
          path: TESTNET
        },

      ];

      for (const testCase of testCases) {
        const { account, expected_address, expected_pk } = testCase;

        const resp = await app.getInfoMultisigVestingAccount(testCase.path, 1, account);
        console.log(resp);

        expect(resp.pubkey?.toString('hex')).toEqual(expected_pk);
        expect(resp.address).toEqual(expected_address);
      }

    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(models)('get vesting address', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      const pubKey0 = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f8';
      const pubKey2 = '8ed90420802c83b41e4a7fa94ce5f05792ea8bff3d7a63572e5c73454eaef51d';
      const pubKey3 = '11bba3ed1721948cefb4e50b0a0bb5cad8a6b52dc7b1a40f4f6652105c91e2c4';

      const testCases = [
        {
          account: {
            pubkeys: [addressToBuffer(pubKey0, 0)],
            approvers: 2,
            participants: 2,
            id: AccountType.Vesting
          },
          expected_address: 'sm1qqqqqq8987egzymqzujln6hpvd649zhf4p5mkdclnwtv0',
          expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
          path: PATH
        },
        {
          account: {
            pubkeys: [
              addressToBuffer(pubKey0, 0),
              addressToBuffer(pubKey3, 3),
              addressToBuffer(pubKey2, 2)
            ],
            approvers: 2,
            participants: 4,
            id: AccountType.Vesting
          },
          expected_address: 'sm1qqqqqq894p92556zmh22f8ywd2ehx0n06qj36pq47u9ey',
          expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
          path: PATH
        },
        {
          account: {
            pubkeys: [
              addressToBuffer(pubKey0, 0),
              addressToBuffer(pubKey3, 3),
              addressToBuffer(pubKey2, 2)
            ],
            approvers: 3,
            participants: 4,
            id: AccountType.Vesting
          },
          expected_address: 'stest1qqqqqq9d46ftwap48q3ydg2w58l5fmy4teh7l5gcmumn0',
          expected_pk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
          path: TESTNET
        }
      ];

      for (const testCase of testCases) {
        const { account, expected_address, expected_pk } = testCase;

        const resp = await app.getInfoMultisigVestingAccount(testCase.path, 1, account);
        console.log(resp);

        expect(resp.pubkey?.toString('hex')).toEqual(expected_pk);
        expect(resp.address).toEqual(expected_address);
      }

    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(models)('get vault address', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      const pubKey0 = '6f1581709bb7b1ef030d210db18e3b0ba1c776fba65d8cdaad05415142d189f8';
      const pubKey2 = '8ed90420802c83b41e4a7fa94ce5f05792ea8bff3d7a63572e5c73454eaef51d';
      const pubKey3 = '11bba3ed1721948cefb4e50b0a0bb5cad8a6b52dc7b1a40f4f6652105c91e2c4';

      const testCases = [
        {
          vaultAccount: {
            owner: {
              pubkeys: [addressToBuffer(pubKey0, 0)],
              approvers: 2,
              participants: 2,
              id: AccountType.Vesting
            },
            totalAmount: BigInt(1000),
            initialUnlockAmount: BigInt(987),
            vestingStart: 567,
            vestingEnd: 99999,
            id: AccountType.Vault
          },
          expected_address: 'sm1qqqqqqqrsssxatqkcxflze4eh8c4rngxsmudk7qqapryj',
          expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
          path: PATH
        },
        {
          vaultAccount: {
            owner: {
              pubkeys: [
                addressToBuffer(pubKey0, 0),
                addressToBuffer(pubKey3, 3),
                addressToBuffer(pubKey2, 2)
              ],
              approvers: 2,
              participants: 4,
              id: AccountType.Vesting
            },
            totalAmount: BigInt(1000),
            initialUnlockAmount: BigInt(987),
            vestingStart: 567,
            vestingEnd: 44,
            id: AccountType.Vault
          },
          expected_address: 'sm1qqqqqqz26rdxe5szj29fjafxm0yyg2mhazq96kg2kds04',
          expected_pk: '136d3aee6442288da85f936f7fe6822186f1d3c63c050721c66bcb7a2095655d',
          path: PATH
        },
        {
          vaultAccount: {
            owner: {
              pubkeys: [
                addressToBuffer(pubKey0, 0),
                addressToBuffer(pubKey3, 3),
                addressToBuffer(pubKey2, 2)
              ],
              approvers: 3,
              participants: 4,
              id: AccountType.Vesting
            },
            totalAmount: BigInt(1000),
            initialUnlockAmount: BigInt(9876543210),
            vestingStart: 567,
            vestingEnd: 99999,
            id: AccountType.Vault
          },
          expected_address: 'stest1qqqqqqpfj2kuy66r8zvkfsdg9xxvy0truqen8lg90z5av',
          expected_pk: 'fa036e263e3351a1365d0355e2c2ccf79b364f686e621418e12c735f87a9d67a',
          path: TESTNET
        },
      ];

      for (const testCase of testCases) {
        const { vaultAccount, expected_address, expected_pk } = testCase;

        const resp = await app.getInfoVaultAccount(testCase.path, 1, vaultAccount, false);
        console.log(resp);
        expect(resp.pubkey?.toString('hex')).toEqual(expected_pk);
        expect(resp.address).toEqual(expected_address);
      }

    } finally {
      await sim.close()
    }
  })

  function addressToBuffer(stringPubKey: string, index: number): Pubkey {
    const bufferPubKey = Buffer.from(stringPubKey, 'hex');
    return { index, pubkey: bufferPubKey };
  }


  // #{TODO} --> Add Zemu tests for different transactions. Include expert mode if needed
  test.concurrent.each(models)('sign blind', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      const responseAddr = await app.getAddressAndPubKey(PATH)
      const pubKey = responseAddr.pubkey

      const test = Buffer.from("This is a dummy message that will be signed by Spacemesh app")
      let singInfo: EdSigner = {
        prefix: Buffer.from("test"),
        domain: Domain.HARE,
        message: test
      };

      // do not wait here.. we need to navigate
      const signatureRequest = app.sign(PATH, singInfo)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_blind`)

      const signatureResponse = await signatureRequest
      console.log(signatureResponse)


      // Now verify the signature
      const payload = Buffer.concat([singInfo.prefix, Buffer.from([singInfo.domain]), singInfo.message]);
      const valid = ed25519.verify(signatureResponse.signature, payload, pubKey)
      expect(valid).toEqual(true)
    } finally {
      await sim.close()
    }
  })

  // test.concurrent.each(models)('sign tx1 normal', async function (m) {
  //   const sim = new Zemu(m.path)
  //   try {
  //     await sim.start({ ...defaultOptions, model: m.name })
  //     const app = new TemplateApp(sim.getTransport())

  //     const txBlob = Buffer.from(txBlobExample)
  //     const responseAddr = await app.getAddressAndPubKey(accountId)
  //     const pubKey = responseAddr.publicKey

  //     // do not wait here.. we need to navigate
  //     const signatureRequest = app.sign(accountId, txBlob)

  //     // Wait until we are not in the main menu
  //     await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
  //     await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_asset_freeze`,50000)

  //     const signatureResponse = await signatureRequest
  //     console.log(signatureResponse)

  //     expect(signatureResponse.return_code).toEqual(0x9000)
  //     expect(signatureResponse.error_message).toEqual('No errors')

  //     // Now verify the signature
  //     const prehash = Buffer.concat([Buffer.from('TX'), txBlob]);
  //     const valid = ed25519.verify(signatureResponse.signature, prehash, pubKey)
  //     expect(valid).toEqual(true)
  //   } finally {
  //     await sim.close()
  //   }
  // })

  // test.concurrent.each(models)('show address - reject', async function (m) {
  //   const sim = new Zemu(m.path)
  //   try {
  //     await sim.start({
  //       ...defaultOptions,
  //       model: m.name,
  //       rejectKeyword: m.name === 'stax' ? 'Public key' : '',
  //     })
  //     const app = new PolkadotGenericApp(sim.getTransport(), 'dot')

  //     const respRequest = app.getAddress(PATH, DOT_SS58_PREFIX, true)
  //     expect(respRequest).rejects.toMatchObject({
  //       returnCode: 0x6986,
  //       errorMessage: 'Transaction rejected'
  //     })
  //     // Wait until we are not in the main menu
  //     await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
  //     try {
  //       await sim.compareSnapshotsAndReject('.', `${m.prefix.toLowerCase()}-show_address_reject`);
  //     } catch {

  //     }

  //   } finally {
  //     await sim.close()
  //   }
  // })
})
