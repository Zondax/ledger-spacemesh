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
import { PATH, defaultOptions, models } from './common'

import { VAULT_TESTCASES } from './testscases/vault'
import { MULTISIG_TESTCASES } from './testscases/multisig'
import { VESTING_TESTCASES } from './testscases/vesting'
import { WALLET_TESTCASES } from './testscases/wallet'
import { Account, EdSigner, Domain, VaultAccount } from '@zondax/ledger-spacemesh/dist/types'

import { ed25519 } from '@noble/curves/ed25519'

jest.setTimeout(45000)

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
        console.log('getVersion error')
      }
    } finally {
      await sim.close()
    }
  })

  describe.each(WALLET_TESTCASES)('Wallet addresses', function (data) {
    test.concurrent.each(models)(`Wallet path: ${data.path}`, async function (m) {
      const sim = new Zemu(m.path)
      try {
        await sim.start({ ...defaultOptions, model: m.name })
        const app = new SpaceMeshApp(sim.getTransport())

        const resp = await app.getAddressAndPubKey(data.path)
        console.log(resp)

        expect(resp.pubkey.toString('hex')).toEqual(data.expectedPk)
        expect(resp.address).toEqual(data.expectedAddress)
      } finally {
        await sim.close()
      }
    })
  })

  test.concurrent.each(models)('show address - reject', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({
        ...defaultOptions,
        model: m.name,
        rejectKeyword: m.name === 'stax' ? 'QR' : '',
      })
      const app = new SpaceMeshApp(sim.getTransport())

      const respRequest = app.getAddressAndPubKey(PATH, true)

      expect(respRequest).rejects.toMatchObject({
        returnCode: 0x6986,
        errorMessage: 'Transaction rejected',
      })

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      try {
        await sim.compareSnapshotsAndReject('.', `${m.prefix.toLowerCase()}-show_address_reject`)
      } finally {
      }
    } finally {
      await sim.close()
    }
  })

  describe.each(MULTISIG_TESTCASES)('Multisig addresses', function (data) {
    test.concurrent.each(models)(`Multisig test: ${data.idx}`, async function (m) {
      const sim = new Zemu(m.path)
      try {
        await sim.start({ ...defaultOptions, model: m.name })
        const app = new SpaceMeshApp(sim.getTransport())
        const { account, expected_address, expected_pk } = data

        const resp = app.getAddressMultisig(data.path, 1, account as Account)
        // Wait until we are not in the main menu
        await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
        await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-multisig_${data.idx}`)

        const multisigResponse = await resp
        console.log(multisigResponse)

        expect(multisigResponse.pubkey.toString('hex')).toEqual(expected_pk)
        expect(multisigResponse.address).toEqual(expected_address)
      } finally {
        await sim.close()
      }
    })
  })

  describe.each(VESTING_TESTCASES)('Vesting addresses', function (data) {
    test.concurrent.each(models)(`Vesting test: ${data.idx}`, async function (m) {
      const sim = new Zemu(m.path)
      try {
        await sim.start({ ...defaultOptions, model: m.name })
        const app = new SpaceMeshApp(sim.getTransport())
        const { account, expected_address, expected_pk } = data

        const resp = app.getAddressVesting(data.path, 1, account as Account)
        // Wait until we are not in the main menu
        await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
        await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-vesting_${data.idx}`)

        const vestingResponse = await resp
        console.log(vestingResponse)

        expect(vestingResponse.pubkey.toString('hex')).toEqual(expected_pk)
        expect(vestingResponse.address).toEqual(expected_address)
      } finally {
        await sim.close()
      }
    })
  })

  describe.each(VAULT_TESTCASES)('Vault addresses', function (data) {
    test.concurrent.each(models)(`Vault test: ${data.idx}`, async function (m) {
      const sim = new Zemu(m.path)
      try {
        await sim.start({ ...defaultOptions, model: m.name })
        const app = new SpaceMeshApp(sim.getTransport())
        const { account, expected_address, expected_pk } = data

        const resp = app.getAddressVault(data.path, 1, account as VaultAccount)

        // Wait until we are not in the main menu
        await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
        await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-vault_${data.idx}`)

        const vaultResponse = await resp
        console.log(vaultResponse)

        expect(vaultResponse.pubkey.toString('hex')).toEqual(expected_pk)
        expect(vaultResponse.address).toEqual(expected_address)
      } finally {
        await sim.close()
      }
    })
  })

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

      // do not wait here... we need to navigate
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
