<!-- markdownlint-disable MD024 -->
# Spacemesh App

## General structure

The general structure of commands and responses is as follows:

### Commands

| Field   | Type     | Content                | Note |
| :------ | :------- | :--------------------- | ---- |
| CLA     | byte (1) | Application Identifier | 0x45 |
| INS     | byte (1) | Instruction ID         |      |
| P1      | byte (1) | Parameter 1            |      |
| P2      | byte (1) | Parameter 2            |      |
| L       | byte (1) | Bytes in payload       |      |
| PAYLOAD | byte (L) | Payload                |      |

### Response

| Field   | Type     | Content     | Note                     |
| ------- | -------- | ----------- | ------------------------ |
| ANSWER  | byte (?) | Answer      | depends on the command   |
| SW1-SW2 | byte (2) | Return code | see list of return codes |

### Return codes

| Return code | Description             |
| ----------- | ----------------------- |
| 0x6400      | Execution Error         |
| 0x6700      | Wrong buffer length     |
| 0x6982      | Empty buffer            |
| 0x6983      | Output buffer too small |
| 0x6984      | Data is invalid         |
| 0x6986      | Command not allowed     |
| 0x6987      | Tx is not initialized   |
| 0x6B00      | P1/P2 are invalid       |
| 0x6D00      | INS not supported       |
| 0x6E00      | CLA not supported       |
| 0x6F00      | Unknown                 |
| 0x6F01      | Sign / verify error     |
| 0x9000      | Success                 |

---

## Command definition

### GET_DEVICE_INFO

#### Command

| Field | Type     | Content                | Expected |
| ----- | -------- | ---------------------- | -------- |
| CLA   | byte (1) | Application Identifier | 0xE0     |
| INS   | byte (1) | Instruction ID         | 0x01     |
| P1    | byte (1) | Parameter 1            | 0x00     |
| P2    | byte (1) | Parameter 2            | 0x00     |
| L     | byte (1) | Bytes in payload       | 0x00     |

#### Response

| Field     | Type     | Content            | Note                     |
| --------- | -------- | ------------------ | ------------------------ |
| TARGET_ID | byte (4) | Target Id          |                          |
| OS_LEN    | byte (1) | OS version length  | 0..64                    |
| OS        | byte (?) | OS version         | Non terminated string    |
| FLAGS_LEN | byte (1) | Flags length       | 0                        |
| MCU_LEN   | byte (1) | MCU version length | 0..64                    |
| MCU       | byte (?) | MCU version        | Non terminated string    |
| SW1-SW2   | byte (2) | Return code        | see list of return codes |

---

### GET_VERSION

#### Command

| Field | Type     | Content                | Expected |
| ----- | -------- | ---------------------- | -------- |
| CLA   | byte (1) | Application Identifier | 0x45     |
| INS   | byte (1) | Instruction ID         | 0x00     |
| P1    | byte (1) | Parameter 1            | ignored  |
| P2    | byte (1) | Parameter 2            | ignored  |
| L     | byte (1) | Bytes in payload       | 0        |

#### Response

| Field   | Type     | Content          | Note                            |
| ------- | -------- | ---------------- | ------------------------------- |
| TEST    | byte (1) | Test Mode        | 0xFF means test mode is enabled |
| MAJOR   | byte (2) | Version Major    | 0..65535                        |
| MINOR   | byte (2) | Version Minor    | 0..65535                        |
| PATCH   | byte (2) | Version Patch    | 0..65535                        |
| LOCKED  | byte (1) | Device is locked |                                 |
| SW1-SW2 | byte (2) | Return code      | see list of return codes        |

---

### INS_GET_ADDR

#### Command

| Field   | Type     | Content                   | Expected          |
| ------- | -------- | ------------------------- | ----------------- |
| CLA     | byte (1) | Application Identifier    | 0x45              |
| INS     | byte (1) | Instruction ID            | 0x01              |
| P1      | byte (1) | Request User confirmation | No = 0 / Yes = 1  |
| P2      | byte (1) | Parameter 2               | ignored           |
| L       | byte (1) | Bytes in payload          | 20                |
| Path[0] | byte (4) | Derivation Path Data      | 0x80000000 \| 44  |
| Path[1] | byte (4) | Derivation Path Data      | 0x80000000 \| 540 |
| Path[2] | byte (4) | Derivation Path Data      | ?                 |
| Path[3] | byte (4) | Derivation Path Data      | ?                 |
| Path[4] | byte (4) | Derivation Path Data      | ?                 |

#### Response

| Field   | Type      | Content     | Note                     |
| ------- | --------- | ----------- | ------------------------ |
| PK      | byte (32) | Public Key  |                          |
| ADDR    | byte (??) | address     |                          |
| SW1-SW2 | byte (2)  | Return code | see list of return codes |

---

### INS_SIGN

#### Command

| Field | Type     | Content                | Expected  |
| ----- | -------- | ---------------------- | --------- |
| CLA   | byte (1) | Application Identifier | 0x45      |
| INS   | byte (1) | Instruction ID         | 0x02      |
| P1    | byte (1) | Payload desc           | 0 = init  |
|       |          |                        | 1 = add   |
|       |          |                        | 2 = last  |
| P2    | byte (1) | ----                   | not used  |
| L     | byte (1) | Bytes in payload       | (depends) |

The first packet/chunk includes only the derivation path

All other packets/chunks contain data chunks that are described below

##### First Packet

| Field   | Type     | Content              | Expected |
| ------- | -------- | -------------------- | -------- |
| Path[0] | byte (4) | Derivation Path Data | 44       |
| Path[1] | byte (4) | Derivation Path Data | 540      |
| Path[2] | byte (4) | Derivation Path Data | ?        |
| Path[3] | byte (4) | Derivation Path Data | ?        |
| Path[4] | byte (4) | Derivation Path Data | ?        |

##### Other Chunks/Packets

| Field   | Type     | Content         | Expected |
| ------- | -------- | --------------- | -------- |
| Message | bytes... | Message to Sign |          |

#### Response

| Field   | Type      | Content     | Note                     |
| ------- | --------- | ----------- | ------------------------ |
| SIG     | byte (65) | Signature   |                          |
| SW1-SW2 | byte (2)  | Return code | see list of return codes |

---

### INS_GET_ADDR_MULTISIG

#### Command

| Field          | Type          | Content                   | Expected          |
| -------------- | ------------- | ------------------------- | ----------------- |
| CLA            | byte (1)      | Application Identifier    | 0x45              |
| INS            | byte (1)      | Instruction ID            | 0x03              |
| P1             | byte (1)      | Payload desc              | 0 = init          | 
|                |               |                           | 1 = add           |
|                |               |                           | 2 = last          |
| P2             | byte (1)      | Parameter 2               | ignored           |
| L              | byte (1)      | Bytes in payload          | 20                |
| Path[0]        | byte (4)      | Derivation Path Data      | 0x80000000 \| 44  |
| Path[1]        | byte (4)      | Derivation Path Data      | 0x80000000 \| 540 |
| Path[2]        | byte (4)      | Derivation Path Data      | ?                 |
| Path[3]        | byte (4)      | Derivation Path Data      | ?                 |
| Path[4]        | byte (4)      | Derivation Path Data      | ?                 |
| internal_index | byte (1)      | internal index            |                   |
| Account        | Account Vault | Account Vault             | ? bytes           |

#### Response

| Field   | Type      | Content     | Note                     |
| ------- | --------- | ----------- | ------------------------ |
| PK      | byte (32) | Public Key  |                          |
| ADDR    | byte (24) | address     | multisig address         |
| SW1-SW2 | byte (2)  | Return code | see list of return codes |

---

### INS_GET_ADDR_VESTING

| Field          | Type          | Content                   | Expected          |
| -------------- | ------------- | ------------------------- | ----------------- |
| CLA            | byte (1)      | Application Identifier    | 0x45              |
| INS            | byte (1)      | Instruction ID            | 0x04              |
| P1             | byte (1)      | Payload desc              | 0 = init          | 
|                |               |                           | 1 = add           |
|                |               |                           | 2 = last          |
| P2             | byte (1)      | Parameter 2               | ignored           |
| L              | byte (1)      | Bytes in payload          | 20                |
| Path[0]        | byte (4)      | Derivation Path Data      | 0x80000000 \| 44  |
| Path[1]        | byte (4)      | Derivation Path Data      | 0x80000000 \| 540 |
| Path[2]        | byte (4)      | Derivation Path Data      | ?                 |
| Path[3]        | byte (4)      | Derivation Path Data      | ?                 |
| Path[4]        | byte (4)      | Derivation Path Data      | ?                 |
| internal_index | byte (1)      | internal index            |                   |
| Account        | Account Vault | Account Vault             | ? bytes           |

#### Response

| Field   | Type      | Content     | Note                     |
| ------- | --------- | ----------- | ------------------------ |
| PK      | byte (32) | Public Key  |                          |
| ADDR    | byte (24) | address     | vesting address          |
| SW1-SW2 | byte (2)  | Return code | see list of return codes |

### INS_GET_ADDR_VAULT

| Field          | Type          | Content                   | Expected          |
| -------------- | ------------- | ------------------------- | ----------------- |
| CLA            | byte (1)      | Application Identifier    | 0x45              |
| INS            | byte (1)      | Instruction ID            | 0x05              |
| P1             | byte (1)      | Payload desc              | 0 = init          | 
|                |               |                           | 1 = add           |
|                |               |                           | 2 = last          |
| P2             | byte (1)      | Parameter 2               | ignored           |
| L              | byte (1)      | Bytes in payload          | 20                |
| Path[0]        | byte (4)      | Derivation Path Data      | 0x80000000 \| 44  |
| Path[1]        | byte (4)      | Derivation Path Data      | 0x80000000 \| 540 |
| Path[2]        | byte (4)      | Derivation Path Data      | ?                 |
| Path[3]        | byte (4)      | Derivation Path Data      | ?                 |
| Path[4]        | byte (4)      | Derivation Path Data      | ?                 |
| internal_index | byte (1)      | internal index            |                   |
| AccountVault   | Account Vault | Account Vault             | ? bytes           |

#### Response

| Field   | Type      | Content     | Note                     |
| ------- | --------- | ----------- | ------------------------ |
| PK      | byte (32) | Public Key  |                          |
| ADDR    | byte (24) | address     | vault address            |
| SW1-SW2 | byte (2)  | Return code | see list of return codes |

### INS_SIGN_MESSAGE

#### Command

| Field   | Type     | Content                | Expected |
| ------- | -------- | ---------------------- | -------- |
| CLA     | byte (1) | Application Identifier | 0x45     |
| INS     | byte (1) | Instruction ID         | 0x06     |
| P1      | byte (1) | Payload desc           | 0 = init  |
|         |          |                        | 1 = add   |
|         |          |                        | 2 = last  |
| P2      | byte (1) | ----                   | not used  |
| L       | byte (1) | Bytes in payload       | (depends) |

The first packet/chunk includes only the derivation path

All other packets/chunks contain data chunks that are described below

##### First Packet

| Field   | Type     | Content              | Expected |
| ------- | -------- | -------------------- | -------- |
| Path[0] | byte (4) | Derivation Path Data | 44       |
| Path[1] | byte (4) | Derivation Path Data | 540      |
| Path[2] | byte (4) | Derivation Path Data | ?        |
| Path[3] | byte (4) | Derivation Path Data | ?        |
| Path[4] | byte (4) | Derivation Path Data | ?        |

##### Other Chunks/Packets

| Field       | Type          | Content                  | Expected |
| ----------- | ------------- | ------------------------ | -------- |
| Prefix len  | byte (2)      | Prefix length            | ?        |
| Message len | byte (2)      | Message length           | ?        |
| DataToSign  | Data to sign  | Data that will be signed | ? bytes  |

#### Response

| Field   | Type      | Content     | Note                     |
| ------- | --------- | ----------- | ------------------------ |
| SIG     | byte (65) | Signature   |                          |
| SW1-SW2 | byte (2)  | Return code | see list of return codes |

### Other structures

#### PubkeyItem

| Field  | Type    | Content    | Note                |
| ------ | ------- | ---------- | ------------------- |
| idx    | u8      | Index      | Index of the pubkey |
| pubkey | u8 (32) | Public Key | 32-byte public key  |

#### Account

| Field        | Type                        | Content                | Note                  |
| ------------ | --------------------------- | ---------------------- | --------------------- |
| approvers    | u8 (1)                      | Number of approvers    | 1 byte                |
| participants | u8 (1)                      | Number of participants | 1 byte                |
| pubkey item  | PubkeyItem (participants-1) | Public key items       | Array of pubkey items |

#### AccountVault

| Field               | Type    | Content               | Note    |
| ------------------- | ------- | --------------------- | ------- |
| totalAmount         | u64     | Total amount          | 8 bytes |
| initialUnlockAmount | u64     | Initial unlock amount | 8 bytes |
| vestingStart        | u32     | Vesting start time    | 4 bytes |
| vestingEnd          | u32     | Vesting end time      | 4 bytes |
| account             | Account | owner account         | ? bytes |

#### Data to sign

| Field         | Type      | Content         | Expected    |
| ------------- | --------- | --------------- | ----------- |
| prefix        | bytes...  | Prefix          | bytes...    |
| domain        | byte (1)  | Domain          | Domain enum |
| message       | bytes...  | Message         | bytes...    |

#### Domain enum

| Name                | Value |
| ------------------- | ----- |
| ATX                 | 0     |
| PROPOSAL            | 1     |
| BALLOT              | 2     |
| HARE                | 3     |
| POET                | 4     |
| BEACON FIRST MSG    | 10    |
| BEACON FOLLOWUP MSG | 11    |