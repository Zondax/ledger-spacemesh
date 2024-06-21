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

import { VAULT_TRANSACTIONS } from './testscases/vault'
import { MULTISIG_TRANSACTIONS } from './testscases/multisig'
import { VESTING_TRANSACTIONS } from './testscases/vesting'
import { WALLET_TRANSACTIONS } from './testscases/wallet'

import { ed25519 } from '@noble/curves/ed25519'

jest.setTimeout(45000)

describe.each(WALLET_TRANSACTIONS)('Wallet transactions', function (data) {
test.concurrent.each(models)('sign', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new SpaceMeshApp(sim.getTransport())

      const responseAddr = await app.getAddressAndPubKey(PATH)
      const pubKey = responseAddr.pubkey
      const messageToSign = Buffer.from(data.blob, 'hex')

      // do not wait here... we need to navigate
      const signatureRequest = app.sign(PATH, messageToSign)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_${data.name}`)

      
      const signatureResponse = await signatureRequest
      console.log(signatureResponse)

      // Now verify the signature
      const valid = ed25519.verify(signatureResponse.signature, messageToSign, pubKey)
      expect(valid).toEqual(true)
    } finally {
      await sim.close()
    }
  })
})

describe.each(MULTISIG_TRANSACTIONS)('Multisig transactions', function (data) {
  test.concurrent.each(models)('sign', async function (m) {
      const sim = new Zemu(m.path)
      try {
        await sim.start({ ...defaultOptions, model: m.name })
        const app = new SpaceMeshApp(sim.getTransport())
  
        const responseAddr = await app.getAddressAndPubKey(PATH)
        const pubKey = responseAddr.pubkey
        const messageToSign = Buffer.from(data.blob, 'hex')
  
        // do not wait here... we need to navigate
        const signatureRequest = app.sign(PATH, messageToSign)
  
        // Wait until we are not in the main menu
        await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
        await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_${data.name}`)
  
        
        const signatureResponse = await signatureRequest
        console.log(signatureResponse)
  
        // Now verify the signature
        const valid = ed25519.verify(signatureResponse.signature, messageToSign, pubKey)
        expect(valid).toEqual(true)
      } finally {
        await sim.close()
      }
    })
  })

  describe.each(VESTING_TRANSACTIONS)('Vesting transactions', function (data) {
    test.concurrent.each(models)('sign', async function (m) {
        const sim = new Zemu(m.path)
        try {
          await sim.start({ ...defaultOptions, model: m.name })
          const app = new SpaceMeshApp(sim.getTransport())
    
          const responseAddr = await app.getAddressAndPubKey(PATH)
          const pubKey = responseAddr.pubkey
          const messageToSign = Buffer.from(data.blob, 'hex')
    
          // do not wait here... we need to navigate
          const signatureRequest = app.sign(PATH, messageToSign)
    
          // Wait until we are not in the main menu
          await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
          await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_${data.name}`)
    
          
          const signatureResponse = await signatureRequest
          console.log(signatureResponse)
    
          // Now verify the signature
          const valid = ed25519.verify(signatureResponse.signature, messageToSign, pubKey)
          expect(valid).toEqual(true)
        } finally {
          await sim.close()
        }
      })
    })

  describe.each(VAULT_TRANSACTIONS)('Vault transactions', function (data) {
    test.concurrent.each(models)('sign', async function (m) {
        const sim = new Zemu(m.path)
        try {
          await sim.start({ ...defaultOptions, model: m.name })
          const app = new SpaceMeshApp(sim.getTransport())
    
          const responseAddr = await app.getAddressAndPubKey(PATH)
          const pubKey = responseAddr.pubkey
          const messageToSign = Buffer.from(data.blob, 'hex')
    
          // do not wait here... we need to navigate
          const signatureRequest = app.sign(PATH, messageToSign)
    
          // Wait until we are not in the main menu
          await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
          await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_${data.name}`)
    
          
          const signatureResponse = await signatureRequest
          console.log(signatureResponse)
    
          // Now verify the signature
          const valid = ed25519.verify(signatureResponse.signature, messageToSign, pubKey)
          expect(valid).toEqual(true)
        } finally {
          await sim.close()
        }
      })
    })