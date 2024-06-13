import { Account, AccountType, PubkeyItem, VaultAccount } from './types'

describe('Account', () => {
  it('should create an Account instance and check sanity', () => {
    const pubkeys: PubkeyItem[] = [
      { index: 0, pubkey: Buffer.alloc(32) },
      { index: 1, pubkey: Buffer.alloc(32) },
    ]
    const account = new Account(AccountType.Wallet, 2, 1, pubkeys)
    expect(account.type).toBe(AccountType.Wallet)
    expect(account.participants).toBe(2)
    expect(account.approvers).toBe(1)
    expect(account.pubkeys).toEqual(pubkeys)
  })

  it('should throw an error for duplicate pubkey indices', () => {
    const pubkeys: PubkeyItem[] = [
      { index: 0, pubkey: Buffer.alloc(32) },
      { index: 0, pubkey: Buffer.alloc(32) },
    ]
    expect(() => new Account(AccountType.Wallet, 2, 1, pubkeys)).toThrow('Duplicate index 0 found in pubkeys array')
  })

  it('should throw an error for invalid pubkey size', () => {
    const pubkeys: PubkeyItem[] = [
      { index: 0, pubkey: Buffer.alloc(31) },
      { index: 1, pubkey: Buffer.alloc(32) },
    ]
    expect(() => new Account(AccountType.Wallet, 2, 1, pubkeys)).toThrow('Invalid pubkey size for')
  })

  it('should throw an error if approvers is 0', () => {
    const pubkeys: PubkeyItem[] = [
      { index: 0, pubkey: Buffer.alloc(32) },
      { index: 1, pubkey: Buffer.alloc(32) },
    ]
    expect(() => new Account(AccountType.Wallet, 2, 0, pubkeys)).toThrow('Approvers cannot be 0')
  })

  it('should throw an error if participants are less than approvers', () => {
    const pubkeys: PubkeyItem[] = [
      { index: 0, pubkey: Buffer.alloc(32) },
      { index: 1, pubkey: Buffer.alloc(32) },
    ]
    expect(() => new Account(AccountType.Wallet, 2, 3, pubkeys)).toThrow('Approvers cannot exceed the number of participants')
  })
})

describe('VaultAccount', () => {
  it('should create a VaultAccount instance and check sanity', () => {
    const pubkeys: PubkeyItem[] = [
      { index: 0, pubkey: Buffer.alloc(32) },
      { index: 1, pubkey: Buffer.alloc(32) },
    ]
    const vaultAccount = new VaultAccount(2, 1, pubkeys, BigInt(500), BigInt(1500), 0, 10)
    expect(vaultAccount.type).toBe(AccountType.Vault)
    expect(vaultAccount.participants).toBe(2)
    expect(vaultAccount.approvers).toBe(1)
    expect(vaultAccount.pubkeys).toEqual(pubkeys)
    expect(vaultAccount.totalAmount).toBe(BigInt(500))
    expect(vaultAccount.initialUnlockAmount).toBe(BigInt(1500))
    expect(vaultAccount.vestingStart).toBe(0)
    expect(vaultAccount.vestingEnd).toBe(10)
  })

  it('should throw an error if initial unlock amount is greater than total amount', () => {
    const pubkeys: PubkeyItem[] = [
      { index: 0, pubkey: Buffer.alloc(32) },
      { index: 1, pubkey: Buffer.alloc(32) },
    ]

    const totalAmount = BigInt(500)
    const initialUnlockAmount = BigInt(1000)

    expect(() => new VaultAccount(2, 1, pubkeys, totalAmount, initialUnlockAmount, 0, 10)).toThrow(
      'Total amount cannot be less than initial unlock amount'
    )
  })

  it('should throw an error if vesting start is greater than vesting end', () => {
    const pubkeys: PubkeyItem[] = [
      { index: 0, pubkey: Buffer.alloc(32) },
      { index: 1, pubkey: Buffer.alloc(32) },
    ]

    const totalAmount = BigInt(1500)
    const initialUnlockAmount = BigInt(1000)

    expect(() => new VaultAccount(2, 1, pubkeys, totalAmount, initialUnlockAmount, 10, 0)).toThrow(
      'Vesting start cannot be greater than vesting end'
    )
  })
})
