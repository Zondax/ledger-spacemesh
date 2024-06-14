import { Account, AccountInterface, VaultAccount } from '@zondax/ledger-spacemesh'

export interface TestCase {
  idx: number
  account: AccountInterface

  expected_address: string
  expected_pk: string
  expected_error?: string

  path: string
}

export class NoCheckAccount extends Account {
  checkSanity() {
    // Do nothing
  }
}

export class NoCheckVaultAccount extends VaultAccount {
  checkSanity() {
    // Do nothing
  }
}
