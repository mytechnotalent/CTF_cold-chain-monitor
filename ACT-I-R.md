# OPERATION COLD IRON - Requirements & Grading Criteria

```
+--------------------------------------------------------------------------------+
|                                                                                |
|                    OPERATION COLD IRON                                         |
|                                                                                |
|                 REQUIREMENTS & GRADING CRITERIA                                |
|                                                                                |
|   TARGET: NorthPharma Depot 7 cold store                                       |
|   ARTIFACT: ACT-I.bin / ACT-I.uf2 (compromised)                                |
|   CREW: FROSTLINE            OPERATIVE: NIGHTINGALE                            |
|                                                                                |
+--------------------------------------------------------------------------------+
```

---

## Project Overview

NorthPharma runs the cold chain that keeps a vaccine lot viable. Each cold
store is watched by a Pico 2 Cold Chain Monitor node. A sabotage crew called
**FROSTLINE** planted an implant called **WHITEOUT** in the monitor image at
Depot 7: a lying HMI, two open doors, and broken authenticated telemetry.
Operative **NIGHTINGALE** recovered the compromised image as `ACT-I.bin`.

Students are the reverse-engineering reserve. They reverse engineer
`ACT-I.bin` with Ghidra, find and patch all six defects, capture the runtime
field key live in GDB, recover the provisioned passphrase and salt, export a
corrected image, flash it to a real Pico 2, and prove the corrected behavior
on the breadboard. The machine check is `scripts/verify_ctf.py`.

The challenge is a standalone capstone exercise and contains no answer,
constant, address, bug, or patch belonging to any other course assignment.

---

## Learning Objectives

- Decode an ARM Cortex-M33 vector and boot table and identify the reset handler
  and initial stack pointer.
- Map a stripped firmware image into modules by tracing calls from `main` and
  the monitor loop.
- Locate six corrupted bytes: a threshold immediate, an annunciator branch, a
  servo pulse constant, a remote-address gate, a tag-compare branch, and a
  nonce loop bound.
- Analyze Thumb immediate values, modified immediates, condition codes, and
  branch inversion.
- Capture a runtime-derived key with GDB and match it to an offline Argon2id
  derivation.
- Recover embedded provisioning secrets and state the OTP production
  requirement.

Students must use only the course concepts: ARM registers, stack behavior,
USB-CDC and UART consoles, GDB, Ghidra static analysis and binary patching,
vector tables, reset startup, XIP, Thumb addressing, data segments and literal
pools, condition-code analysis, runtime key derivation, and the Argon2id plus
XChaCha20-Poly1305 authenticated envelope.

---

## Deliverables Checklist

| # | Deliverable | Format | Criterion |
|---|-------------|--------|-----------|
| 1 | Ghidra project screenshot | PNG/JPG | Task 1 |
| 2 | Vector table and boot table | Inside `ACT-I-Answers.md` | Task 1 |
| 3 | `main` and monitor-loop table | Inside `ACT-I-Answers.md` | Task 1 |
| 4 | Module map | Inside `ACT-I-Answers.md` | Task 1 |
| 5 | Cold offset evidence and patch | Inside `ACT-I-Answers.md` | Task 2 |
| 6 | Dead annunciator evidence and patch | Inside `ACT-I-Answers.md` | Task 3 |
| 7 | Damper geometry evidence and patch | Inside `ACT-I-Answers.md` | Task 4 |
| 8 | Infrared door evidence and patch | Inside `ACT-I-Answers.md` | Task 5 |
| 9 | Constant tag evidence and patch | Inside `ACT-I-Answers.md` | Task 6 |
| 10 | Nonce reuse evidence and patch | Inside `ACT-I-Answers.md` | Task 7 |
| 11 | GDB key capture | Inside `ACT-I-Answers.md` | Task 8 |
| 12 | Provisioning recovery | Inside `ACT-I-Answers.md` | Task 9 |
| 13 | `ACT-I_fixed.bin` | BIN file | Task 10 |
| 14 | `ACT-I_fixed.uf2` | UF2 file | Task 10 |
| 15 | Hardware proof and reflection | Inside `ACT-I-Answers.md` | Task 10 |

---

## Required Tools and Equipment

| Tool | Purpose |
|------|---------|
| Raspberry Pi Pico 2 | Isolated target node |
| Debug Probe (OpenOCD) | SWD connection for GDB inspection |
| arm-none-eabi-gdb | Runtime breakpoints and key capture |
| Ghidra | Static analysis and binary patching |
| Python 3 with `uf2conv.py` | UF2 conversion and artifact checks |
| DHT11, 1602 I2C LCD, RYLR998, IR receiver, SG90 servo, 3 LEDs, button | Breadboard hardware proof |
| `ACT-I.bin` and `ACT-I.uf2` | Supplied compromised artifacts |

Console settings: **USB-CDC virtual COM port, 115200 baud, 8 data bits, no
parity, 1 stop bit**. Radio UART settings: **UART1, 115200, network ID 18**.

---

## Artifact Identity

The instructor-issued artifact hashes are:

```text
ACT-I.bin        b6fced1c0c305fdf167ed2da93464997504e1126d552eb120016d34c3a0bc47a
ACT-I.uf2        015e31dfeec6728afbc3a606d8b7b0283666f994acf124e56d0ed182119033f4
ACT-I_fixed.bin  e63c513a8b798c0e4be9265e4313b8d882f3b6fd8e9ae1161316adad7a926e29
ACT-I_fixed.uf2  f5df73b6ca69d4384ae64ffc39920e8d36634639a12b23904a962bd743332bf8
```

The verifier checks the `ACT-I.bin` and `ACT-I.uf2` hashes specifically and
asserts the six fixed bytes in `ACT-I_fixed.bin`.

---

## Grading Rubric - Detailed Breakdown

### Task 1: Setup and Initial Analysis (10 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Ghidra project created with the correct name and settings | 2 | Project `ColdIron_Investigation`, raw binary import | One item off | Not set up |
| **[DOCUMENT]** Processor configured as ARM Cortex 32 little endian default | 2 | Screenshot shows the correct processor | Wrong language | Missing |
| **[DOCUMENT]** Base address set to 0x10000000 | 2 | Base `0x10000000` | Wrong base | Missing |
| **[DOCUMENT]** Vector table, initial stack pointer, and reset handler identified | 2 | Base `0x10000000`, initial SP `0x20082000`, reset handler `0x1000015B` | One missing | Not found |
| **[DOCUMENT]** main and the monitor loop (monitor_step) addresses identified | 1 | `main` `0x100001E0`, `monitor_step` `0x10006518` | One correct | Neither |
| **[DOCUMENT]** Module map identifies the sensor, display, radio, status LEDs, button, servo, infrared, envelope, crypto, and KDF anchors | 1 | At least one correct anchor per module | Partial | Missing |

### Task 2: Bug #1 Cold Offset (12 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Located the cold offset compare at 0x1000759E | 3 | Address and function identified | Approximate | Not found |
| **[DOCUMENT]** Documented the compromised immediate and the correct threshold | 3 | Compromised `0xFA` (250), correct `0x00` (0) | Correct address, no values | Wrong values |
| **[DOCUMENT & PATCH]** Patched 0xFA to 0x00 so a warm reading classifies as a breach | 4 | Byte `0xFA` changed to `0x00` | Wrong byte | Not patched |
| **[DOCUMENT]** Explained that a 5.0 C reading (50 tenths) now alarms | 2 | 50 tenths at or above 0.0 C, so BREACH | Vague | Missing |

### Task 3: Bug #2 Dead Annunciator (12 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Located the annunciator branch at 0x1000754F | 3 | Address and function identified | Approximate | Not found |
| **[DOCUMENT]** Documented beq.n versus bne.n and the branch target | 3 | Correct branch and target `0x1000757E` | Partial | Wrong |
| **[DOCUMENT & PATCH]** Patched 0xD1 to 0xD0 so a breach routes to the red lamp | 4 | Byte `0xD1` changed to `0xD0` | Wrong byte | Not patched |
| **[DOCUMENT]** Explained why the compromised branch leaves the HMI looking healthy | 2 | Breach falls into the green nominal path | Vague | Missing |

### Task 4: Bug #3 Damper Geometry (12 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Located the damper seal pulse constant at 0x10007686 | 3 | Address and function identified | Approximate | Not found |
| **[DOCUMENT]** Documented the 800 versus 500 microsecond pulse and its angle | 3 | Compromised 800 us near 27 degrees, correct 500 us | Partial | Wrong |
| **[DOCUMENT & PATCH]** Patched 0x48 to 0xFA so the damper seals | 4 | Byte `0x48` changed to `0xFA` | Wrong byte | Not patched |
| **[DOCUMENT]** Explained why the geometry error is a physical failure | 2 | The damper never fully closes so the store warms | Vague | Missing |

### Task 5: Bug #4 Open Infrared Door (12 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Located the infrared address gate at 0x10006569 | 3 | Address and inlined handler identified | Approximate | Not found |
| **[DOCUMENT]** Documented the authorized address and the inverted branch | 3 | Address `0x1D` (29), compromised `beq.n` | Partial | Wrong |
| **[DOCUMENT & PATCH]** Patched 0xD0 to 0xD1 so only the maintenance address is accepted | 4 | Byte `0xD0` changed to `0xD1` | Wrong byte | Not patched |
| **[DOCUMENT]** Explained why the compromised gate accepts every address | 2 | The reject branch is taken only on a match | Vague | Missing |

### Task 6: Bug #5 Constant Tag (12 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Located the tag-difference branch at 0x10007AFF | 3 | Address and function identified | Approximate | Not found |
| **[DOCUMENT]** Documented the constant-time compare and the branch condition | 3 | Zero difference means accept | Partial | Wrong |
| **[DOCUMENT & PATCH]** Patched 0xD1 to 0xD0 so forged frames fail authentication | 4 | Byte `0xD1` changed to `0xD0` | Wrong byte | Not patched |
| **[DOCUMENT]** Explained why a non-zero tag difference must be rejected | 2 | A non-zero difference means forged or corrupt | Vague | Missing |

### Task 7: Bug #6 Nonce Reuse (12 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Located the nonce fill loop bound at 0x10007858 | 3 | Address and function identified | Approximate | Not found |
| **[DOCUMENT]** Documented the compromised and correct loop bounds | 3 | Compromised `0x00`, correct `0x18` (24) | Partial | Wrong |
| **[DOCUMENT & PATCH]** Patched 0x00 to 0x18 so 24 random bytes are drawn | 4 | Byte `0x00` changed to `0x18` | Wrong byte | Not patched |
| **[DOCUMENT]** Explained nonce reuse and how a fresh random nonce stops it | 2 | Zero iterations means a fixed nonce; 24 random bytes fixes it | Vague | Missing |

### Task 8: Runtime Key Capture (8 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Breakpoint set at 0x100064CE after the crypto_kdf_argon2id call | 3 | Correct breakpoint address and command sequence | Address off | Not found |
| **[DOCUMENT]** Captured the 32-byte key from SRAM 0x20013074 | 3 | 32-byte hex read from `g_key` | Partial bytes | Missing |
| **[DOCUMENT]** Compared the captured key to the gateway Argon2id derivation | 2 | Captured key equals the derived key | Partial | Missing |

### Task 9: Provisioning Recovery (5 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[DOCUMENT]** Recovered the passphrase and salt from flash | 2 | `operation cold iron field key v1` and `coldiron-salt-01` | One recovered | Not found |
| **[DOCUMENT]** Explained that committed secrets are dev-only and production must use OTP | 1 | OTP provisioning requirement stated | Vague | Missing |
| **[DOCUMENT]** Re-derived the key offline with Argon2id and matched the Task 8 capture | 2 | t=3, p=1, m=64 and a match | Partial | Missing |

### Task 10: Export and Verify (5 points)

| Criterion | Points | Full credit | Partial credit | No credit |
|-----------|--------|-------------|----------------|-----------|
| **[PATCH]** Exported ACT-I_fixed.bin from Ghidra | 1 | Valid patched binary | Corrupt | Not submitted |
| **[PATCH]** Converted to ACT-I_fixed.uf2 with the correct base and family | 1 | `--base 0x10000000 --family 0xe48bff59` | Wrong flags | Not submitted |
| **[DOCUMENT]** scripts/verify_ctf.py passes and hardware proves the red lamp, damper seal, and authentication | 2 | Verifier passes and the hardware proof is shown | Partial proof | No proof |
| **[DOCUMENT]** Reflection maps each of the six defects to a real-world cold-chain failure | 1 | Specific mapping for all six | Partial | Missing |

---

## Common Pitfalls

| Pitfall | Consequence | Avoidance |
|---------|-------------|-----------|
| Editing the wrong byte at `0x759E` | Warm reading still nominal | The low byte is `0xFA`; change it to `0x00` |
| Confusing `beq` and `bne` at `0x754F` | Red lamp still dark | Fixed is `beq.n` (`0xD0`) to the red path |
| Treating the damper pulse as degrees | Wrong byte patched | The immediate is a pulse; `500` us is the seal |
| Missing that the IR handler is inlined | Cannot find the gate | Look inside `monitor_step` at `0x10006569` |
| Reading the tag branch backwards | Forged frames still pass | Accept only on zero difference (`beq.n`, `0xD0`) |
| Patching the nonce call instead of the bound | Loop still runs zero times | The bound is the immediate at `0x7858` |
| Forgetting UF2 conversion | Raw binary will not flash | Use `uf2conv.py` with family `0xe48bff59` |
| Fabricating the GDB key | Verification fails | The key equals the Argon2id output of Task 9 |

---

## How To Breadboard

| Device | Pin on device | Pico 2 GPIO | Notes |
|--------|---------------|-------------|-------|
| DHT11 data | DATA | GP4 | 10 kOhm pull-up to 3.3 V if the module needs it |
| 1602 LCD | SDA | GP2 | I2C1, backpack address `0x27` |
| 1602 LCD | SCL | GP3 | I2C1, 100 kHz |
| 1602 LCD | VCC / GND | VBUS 5 V / GND | The backpack needs 5 V, not 3.3 V |
| RYLR998 | RX | GP8 (Pico TX) | UART1, 115200, network ID 18 |
| RYLR998 | TX | GP9 (Pico RX) | UART1 |
| IR receiver | OUT | GP5 | VS1838B, internal pull-up enabled |
| Servo | signal | GP14 | PWM 50 Hz; 1000 uF bulk cap across servo 5 V and GND |
| Red LED | anode | GP16 | 220 to 330 ohm to GND |
| Yellow LED | anode | GP17 | 220 to 330 ohm to GND |
| Green LED | anode | GP18 | 220 to 330 ohm to GND |
| Button | leg 1 | GP15 | Internal pull-up; leg 2 to GND, never to 3.3 V |
| Onboard LED | built in | GP25 | Green blink on transmit |
| Debug Probe | SWCLK / SWDIO / GND | debug header | For GDB only |

Use 3.3 V logic on every GPIO. The only 5 V connection is the LCD backpack
supply. Keep the 1000 uF capacitor on the servo rail.

---

## Memory Map Reference

| Region | Address | Purpose |
|--------|---------|---------|
| Bootrom | `0x00000000` | Immutable boot code |
| Flash/XIP | `0x10000000` | Vector table, code, rodata, data image |
| SRAM | `0x20000000` | Stack and writable state |
| Derived key `g_key` | `0x20013074` | 32-byte Argon2id field key |

The VA of any file offset is the file offset plus `0x10000000`.

---

## Deadline & Submission

- Create a folder containing the Ghidra screenshot, `ACT-I_fixed.bin`, and
  `ACT-I_fixed.uf2`.
- Write all written answers in `ACT-I-Answers.md` inside that folder.
- Include the output of `python scripts/verify_ctf.py`.
- ZIP the folder as `lastname-firstname-ACT-I.zip`.
- Submit the ZIP before the posted deadline; late submissions lose 10 percent
  per day.

---

## Grade Scale

| Grade | Percentage | Points |
|-------|------------|--------|
| A+ | 97-100% | 97-100 |
| A  | 93-96% | 93-96 |
| A- | 90-92% | 90-92 |
| B+ | 87-89% | 87-89 |
| B  | 84-86% | 84-86 |
| B- | 80-83% | 80-83 |
| C  | 70-79% | 70-79 |
| F  | 0-69% | 0-69 |

---

## Academic Integrity

Use only the supplied Pico 2 and firmware. Do not connect the exercise to an
operational cold store, a pharmaceutical network, a public network, a military
system, or a third-party device. This is a controlled, isolated educational
exercise. All analysis and patches must be your own work; sharing binaries,
addresses, keys, passphrases, or answers is a violation of the academic
integrity policy.

---

## Reference Material

| Topic | Reference |
|-------|-----------|
| ARM Cortex-M33 registers and stack | Course block 1 |
| USB-CDC and UART console capture | Course block 2 |
| Vector tables, reset startup, and XIP | Course block 3 |
| Ghidra static analysis and binary patching | Course block 4 |
| Condition-code and branch analysis | Course block 5 |
| Servo geometry and actuator math | Course block 6 |
| Argon2id and XChaCha20-Poly1305 authenticated envelope | Course block 7 |
| Runtime key capture and provisioning | Course block 8 |
