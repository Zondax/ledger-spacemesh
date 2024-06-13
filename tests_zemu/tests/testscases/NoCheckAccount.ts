import {Account, VaultAccount} from "@zondax/ledger-spacemesh/dist/types";

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
