# OPERATION COLD IRON CTF - Design Blueprint

Artifact prefix: `ACT-I`
Repo: `11_operation-cold-iron-ctf-c-rp2350`
Author: Kevin Thomas (kevin@mytechnotalent.com)

This document is the build spine for the challenge. It is not student-facing.
Student-facing docs are the canonical `ACT-I-I.md`, `ACT-I-R.md`, and
`ACT-I-S.md` plus their PDFs, enforced by
`.opencode/skill/eh-project-structure/validate_structure.py`.

---

## Why this one is the best of the five

The four existing CTFs and finals each teach one or two lessons. This one fuses
all of them into a single device and adds the two things the others do not have:

1. **Two independent unauthenticated attack surfaces on one target.** The LoRa
   telemetry link and the NEC infrared maintenance port. A student who closes
   only one door still ships poison.
2. **A cryptographic fix that is real cryptography, not a string patch.**
   Students break a deliberately broken AEAD (nonce reuse and a constant tag),
   recover the field key at runtime with GDB, and restore per-frame
   XChaCha20-Poly1305 authentication.
3. **A physical consequence.** The servo drives the cold-store damper, so a
   wrong angle constant is not cosmetic: the vials warm up.
4. **An HMI that lies.** The LEDs and the LCD are sabotaged to report nominal
   while the box is failing, which is the emotional core of the story.

Everything is real firmware on real hardware: Pico 2, Debug Probe, DHT11, 1602
I2C LCD, three status LEDs, a button, an SG90 servo with a 1000uF bulk cap, a
VS1838B infrared receiver with an NEC remote, and an RYLR998 LoRa radio.

---

## Scenario (student-facing framing)

NIGHTINGALE copied the firmware from a depot node whose monitor insisted the
cold store was a perfect minus eighteen degrees while the wall thermometer read
five above zero. The image is compromised by FROSTLINE. You have the binary, the
breadboard, and a debug probe. Find every backdoor, prove each one on hardware,
patch the binary, and restore authenticated telemetry before the next shipment
leaves the dock.

---

## Deliverable layout (canonical, enforced)

```
ACT-I-I.md / .pdf      student instructions
ACT-I-R.md / .pdf      requirements and grading criteria
ACT-I-S.md / .pdf      instructor solution key
ACT-I.bin / .uf2       compromised artifact
ACT-I_fixed.bin / .uf2 student-patched artifact
ACT-I-main-disasm.txt  annotated disassembly
scripts/verify_ctf.py   machine verifier for all patches
scripts/{spoof,gateway}.py  classroom tooling
src/  include/  ghidra/  CMakeLists.txt  pico_sdk_import.cmake
uf2conv.py  uf2families.json
```

The three docs follow the exact canonical skeleton used by the other four
projects: the `-I` has `Project Overview`, `Scenario Briefing`, `Learning
Objectives`, `What This Project Tests`, `Part 1..3`, `Task 1..N`, `How To
Breadboard`, `Memory Map Reference`, `Submission Format`, `Success Criteria`,
`Academic Integrity`, `Reference Material`; the `-R` has the grading skeleton;
the `-S` has `Artifact Identity` then per-task `Solution` / `Grading Rubric
(1-to-1 Mapping)` / `Instructor Notes & Assembly`, then `How To Breadboard`,
`Complete Grading Summary`, `Instructor Notes`, `Appendix: Expected Binary
Diff`. The validator proves parity across all five projects.

---

## The compromised firmware (the artifact students reverse)

The CTF image is the OPERATION COLD IRON node with six deliberate defects and
two recovery objectives. The defects are placed so each maps to a distinct
reverse-engineering skill.

All offsets are file offsets; VA = offset + 0x10000000. The fixed byte is the
correct value, the compromised byte is what ships in `ACT-I.bin`.

| # | Name | Location | Offset | Fixed | Compromised | Fix |
| - | ---- | -------- | ------ | ----- | ----------- | --- |
| 1 | Cold offset | `status_led_state_for_temperature` | 0x759E | 0x00 | 0xFA | `cmp r0, #0` restored (50 tenths -> BREACH) |
| 2 | Dead annunciator | `status_led_show` | 0x754F | 0xD0 | 0xD1 | `beq.n` restored so breach lights red |
| 3 | Damper geometry | `servo_seal` | 0x7686 | 0xFA | 0x48 | seal pulse 500 us restored (was 800) |
| 4 | Open infrared door | inlined gate in `monitor_step` | 0x6569 | 0xD1 | 0xD0 | `bne.n` restored so only the maintenance address passes |
| 5 | Constant tag | `crypto_aead_open` | 0x7AFF | 0xD0 | 0xD1 | `beq.n` restored so only a zero tag difference accepts |
| 6 | Nonce reuse | `envelope_fill_nonce` | 0x7858 | 0x18 | 0x00 | loop bound 24 restored so 24 random bytes are drawn |

Total intended changed bytes: 6. `ACT-I.bin` and `ACT-I_fixed.bin` differ in
exactly those six bytes and nothing else.

Recovery objectives:

| # | Name | Objective | Skill |
| - | ---- | --------- | ----- |
| A | Runtime key capture | Break at the instruction after the `crypto_kdf_argon2id` call in `monitor_init` (`0x100064CE`) and read the 32-byte derived key from SRAM `0x20013074`. | GDB dynamic analysis |
| B | Provisioning recovery | Recover the field passphrase `operation cold iron field key v1` and salt `coldiron-salt-01` from flash, re-derive the key offline with Argon2id, and match the Task A capture. | Static data recovery and crypto |

Recovery objectives (not bugs):

| # | Name | Objective | Skill |
| - | ---- | --------- | ----- |
| A | Runtime key capture | Break at the key-derivation return and read the derived field key from `$r0`. | GDB dynamic analysis |
| B | Quarantined authority frame | Recover the never-transmitted FROSTLINE authority frame and the passphrase that authenticates it. | Static data recovery and crypto |

The crypto core is the same verified in-repo implementation used by OPERATION
COLD IRON: Argon2id (RFC 9106) plus XChaCha20-Poly1305 (RFC 8439), with BLAKE2b
and Poly1305. The CTF deliberately breaks only the two seams above (tag compare
and nonce source), so the rest of the cryptography is a correct reference the
student can trust once the seams are fixed.

---

## Task list and points (100 total)

| Task | Points | Focus |
| ---- | ------ | ----- |
| Task 1: Setup and Initial Analysis | 10 | Ghidra project, vector table, `main`, module map |
| Task 2: Bug #1 Cold Offset | 12 | threshold immediate |
| Task 3: Bug #2 Dead Annunciator | 12 | LED state mapping |
| Task 4: Bug #3 Damper Geometry | 12 | servo seal angle |
| Task 5: Bug #4 Open Infrared Door | 12 | maintenance command validation |
| Task 6: Bug #5 Constant Tag | 12 | AEAD tag comparison |
| Task 7: Bug #6 Nonce Reuse | 12 | nonce source |
| Task 8: Runtime Key Capture | 8 | GDB derived key from SRAM 0x20013074 |
| Task 9: Provisioning Recovery | 5 | recover passphrase/salt from flash, re-derive, cross-check |
| Task 10: Export and Verify | 5 | `ACT-I_fixed.bin`/`.uf2`, hardware proof |
| Written Reflection | included in Task 10 | lessons and fixes |

Tasks 2 through 7 each carry a `[DOCUMENT]` / `[DOCUMENT & PATCH]` criterion
pair so the `-R` and `-S` stay 1-to-1 with a total of 100 points.

---

## Verifier

`scripts/verify_ctf.py` reads `ACT-I.bin` and `ACT-I_fixed.bin`, asserts
exactly the expected byte changes at the expected offsets, asserts the fixed
image differs in exactly the intended number of bytes (no collateral damage),
and asserts the two artifact SHA-256 values. It mirrors the verifiers in the
other four projects and is the objective grading backstop.

---

## Hardware proof

Each student proves the fix on the breadboard:

- Feed a warm reading (or force the DHT path) and watch the red lamp light and
  the damper vent.
- Replay the NEC remote and confirm the fixed firmware rejects the unauthenticated
  VENT command.
- Forge a telemetry frame with `scripts/spoof.py` and confirm the authenticated
  gateway logs `UNAUTHENTICATED` and takes no action.
- Read the derived key with GDB and confirm it matches the value the gateway
  derives from the provisioned passphrase.

---

## Naming and repository

The repo is standalone and self-contained. The artifact prefix is `ACT-I`
(continuing `CTF-01` and `CTF-02`), the display name is OPERATION COLD IRON, and
the author/contact metadata matches the other projects (Kevin Thomas, George
Mason University, kthoma60@gmu.edu).
