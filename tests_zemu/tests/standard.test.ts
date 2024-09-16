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

import Zemu, { zondaxMainmenuNavigation, isTouchDevice, ButtonKind } from '@zondax/zemu'
import { SpaceMeshApp, VaultAccount, Account } from '@zondax/ledger-spacemesh'
import { PATH, defaultOptions, models } from './common'

import { VAULT_TESTCASES } from './testscases/vault'
import { MULTISIG_TESTCASES } from './testscases/multisig'
import { VESTING_TESTCASES } from './testscases/vesting'
import { WALLET_TESTCASES } from './testscases/wallet'

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

        const resp = await app.getAddressAndPubKey(data.path, Buffer.from(data.genesisId, 'hex'), false)
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
        rejectKeyword: isTouchDevice(m.name) ? 'Confirm' : '',
      })
      const app = new SpaceMeshApp(sim.getTransport())

      await sim.toggleExpertMode()

      const respRequest = app.getAddressAndPubKey(PATH, Buffer.from('9eebff023abb17ccb775c602daade8ed708f0a50', 'hex'), true)

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
        // await sim.start({ ...defaultOptions, model: m.name })
        await sim.start({
          ...defaultOptions,
          model: m.name,
          approveKeyword: isTouchDevice(m.name) ? 'Approve' : '',
          approveAction: ButtonKind.DynamicTapButton,
        })

        const app = new SpaceMeshApp(sim.getTransport())
        const { account, expected_address, expected_pk } = data

        const resp = app.getAddressMultisig(data.path, 1, account as Account, Buffer.from(data.genesisId, 'hex'))
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
        await sim.start({
          ...defaultOptions,
          model: m.name,
          approveKeyword: isTouchDevice(m.name) ? 'Approve' : '',
          approveAction: ButtonKind.DynamicTapButton,
        })

        const app = new SpaceMeshApp(sim.getTransport())
        const { account, expected_address, expected_pk } = data

        const resp = app.getAddressVesting(data.path, 1, account as Account, Buffer.from(data.genesisId, 'hex'))
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
        await sim.start({
          ...defaultOptions,
          model: m.name,
          approveKeyword: isTouchDevice(m.name) ? 'Approve' : '',
          approveAction: ButtonKind.DynamicTapButton,
        })

        const app = new SpaceMeshApp(sim.getTransport())
        const { account, expected_address, expected_pk } = data

        const resp = app.getAddressVault(data.path, 1, account as VaultAccount, Buffer.from(data.genesisId, 'hex'))

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
})
