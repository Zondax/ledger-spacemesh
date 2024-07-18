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
import { SpaceMeshApp, Domain, MsgSigner } from '@zondax/ledger-spacemesh'
import { PATH, defaultOptions, models } from './common'

import { ed25519 } from '@noble/curves/ed25519'

jest.setTimeout(45000)

const TESTS = [
  {
    name: 'raw_sign',
    prefix: 'prefix',
    message: 'This is our test payload!',
    domain: Domain.ATX
  },
  {
    name: 'raw_sign_hex',
    prefix: 'hex prefix',
    message: 'This is our test payload with emoji! 😉',
    domain: Domain.HARE
  },
  {
    name: 'raw_sign_empty',
    prefix: '',
    message: '',
    domain: Domain.BEACON_FOLLOWUP_MSG
  },
]

describe.each(TESTS)('Raw signing', function (data) {
test.concurrent.each(models)('sign', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      const msgToSign: MsgSigner = {
        prefix: Buffer.from(data.prefix),
        message: Buffer.from(data.message),
        domain: data.domain
      };

      const responseAddr = await app.getAddressAndPubKey(PATH)
      const pubKey = responseAddr.pubkey

      // do not wait here... we need to navigate
      const signatureRequest = app.signMessage(PATH, msgToSign)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-${data.name}`)

      
      const signatureResponse = await signatureRequest
      console.log(signatureResponse)

      // Now verify the signature
      const payload = Buffer.concat([msgToSign.prefix, Buffer.from([msgToSign.domain]), msgToSign.message])
      const valid = ed25519.verify(signatureResponse.signature, payload, pubKey)
      expect(valid).toEqual(true)
    } finally {
      await sim.close()
    }
  })
})

const TESTS_FAIL = [
  {
    name: 'raw_sign_wrong_domain',
    prefix: 'prefix',
    message: 'This is our test payload!',
    domain: 5,
    expectedError: 'Data is invalid : Unexpected value'
  },
]

describe.each(TESTS_FAIL)('Raw signing - incorrect', function (data) {
  test.concurrent.each(models)('sign', async function (m) {
      const sim = new Zemu(m.path)
      try {
        await sim.start({ ...defaultOptions, model: m.name })
        const app = new SpaceMeshApp(sim.getTransport())
  
        const msgToSign: MsgSigner = {
          prefix: Buffer.from(data.prefix),
          message: Buffer.from(data.message),
          domain: data.domain
        };
  
        const responseAddr = await app.getAddressAndPubKey(PATH)
        const pubKey = responseAddr.pubkey
  
        // do not wait here... we need to navigate
        const signatureRequest = app.signMessage(PATH, msgToSign)  

        await expect(signatureRequest).rejects.toThrow(new Error(data.expectedError))
      } finally {
        await sim.close()
      }
    })
  })
