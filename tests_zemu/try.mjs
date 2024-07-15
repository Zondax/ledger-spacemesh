import TransportNodeHid from '@ledgerhq/hw-transport-node-hid'
import { SpaceMeshApp } from '@zondax/ledger-spacemesh'
import { ed25519 } from '@noble/curves/ed25519'

async function main() {
  const transport = await TransportNodeHid.default.open();

  const app = new SpaceMeshApp(transport);

  const PATH = "m/44'/540'/0'/0'/0'"
  //const PATH_TESTNET = "m/44'/1'/0'/0'/0'"
  const get_resp = await app.getAddressAndPubKey(PATH)
  const pubKey = get_resp.pubkey
  console.log(get_resp)

  let resp = await app.deviceInfo()
  console.log('Device Info', resp);
  resp = await app.getVersion()
  console.log('Version', resp);

  const blobSpend = "9EEBFF023ABB17CCB775C602DAADE8ED708F0A500000000000833816A6695F08E9037763CAE46612DED40F9F2D4000D52400000000719D58B3F3A1C73BECC9140B07A474DE6F5275EF02286BEE"
  const messageToSign = Buffer.from(blobSpend, 'hex')
  const signatureRequest = app.sign(PATH, messageToSign)

  const signatureResponse = await signatureRequest
  console.log(signatureResponse)

  // Now verify the signature
  const valid = ed25519.verify(signatureResponse.signature, messageToSign, pubKey)
  if (valid) {
    console.log("Valid signature")
  } else {
    console.log("Invalid signature")
  }
}

; (async () => {
  await main()
})()