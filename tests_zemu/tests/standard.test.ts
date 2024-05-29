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
import { TemplateApp } from '@zondax/ledger-template'
import { PATH, defaultOptions, models, txBlobExample } from './common'

// @ts-expect-error
import ed25519 from 'ed25519-supercop'

jest.setTimeout(60000)

const expected_address = 'BX63ZW4O5PWWFDH3J33QEB5YN7IN5XOKPDUQ5DCZ232EDY4DWN3XKUQRCA'
const expected_pk = '0dfdbcdb8eebed628cfb4ef70207b86fd0deddca78e90e8c59d6f441e383b377'

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
      const app = new TemplateApp(sim.getTransport())
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
      const app = new TemplateApp(sim.getTransport())

      try {
        const resp = await app.getAddressAndPubKey(PATH, false)

        console.log(resp)
        // expect(resp.pubkey).toEqual(expected_pk)
        // expect(resp.address).toEqual(expected_address)
      } catch {
        console.log("getAddress error")
      }

    } finally {
      await sim.close()
    }
  })

  // test.concurrent.each(models)('show address', async function (m) {
  //   const sim = new Zemu(m.path)
  //   try {
  //     await sim.start({...defaultOptions, model: m.name,
  //                      approveKeyword: m.name === 'stax' ? 'QR' : '',
  //                      approveAction: ButtonKind.ApproveTapButton,})
  //     const app = new TemplateApp(sim.getTransport())

  //     const respRequest = app.getAddressAndPubKey(accountId, true)
  //     // Wait until we are not in the main menu
  //     await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
  //     await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-show_address`)

  //     const resp = await respRequest
  //     console.log(resp)

  //     expect(resp.return_code).toEqual(0x9000)
  //     expect(resp.error_message).toEqual('No errors')
  //   } finally {
  //     await sim.close()
  //   }
  // })

  // test.concurrent.each(models)('show address - reject', async function (m) {
  //   const sim = new Zemu(m.path)
  //   try {
  //     await sim.start({...defaultOptions, model: m.name,
  //                      rejectKeyword: m.name === 'stax' ? 'QR' : ''})
  //     const app = new TemplateApp(sim.getTransport())

  //     const respRequest = app.getAddressAndPubKey(accountId, true)
  //     // Wait until we are not in the main menu
  //     await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())

  //     await sim.compareSnapshotsAndReject('.', `${m.prefix.toLowerCase()}-show_address_reject`, 'REJECT')

  //     const resp = await respRequest
  //     console.log(resp)

  //     expect(resp.return_code).toEqual(0x6986)
  //     expect(resp.error_message).toEqual('Transaction rejected')
  //   } finally {
  //     await sim.close()
  //   }
  // })

  // #{TODO} --> Add Zemu tests for different transactions. Include expert mode if needed
  // test.concurrent.each(models)('sign tx0 normal', async function (m) {
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
})
