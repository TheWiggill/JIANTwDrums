# Bluetooth LE for JIANT FM: feasibility (stage 1)

Status: research only. No firmware file was changed. Date: 2026-10-31.
Goal: (a) BLE MIDI in/out; (b) above all a **proximity sensor**: the RSSI of received BLE packets, smoothed into a
modulation source.

Reference trees (read-only, in the session scratchpad `ext/`): `melodee` at tag `v1.6.0-rc.7` (worktree `melodee-rc7`),
`sloop-fm1` (isod89 main + every PR head), `sloop-fext` (jmarianski/sloop-fm1 branch `fext`), `felucca-fext`
(jmarianski/Felucca `fext`), `fble` (jmarianski), `fm1-base` (luchak main) and `fm1-base-cv` (charlesvestal main,
which has `ble_stack = "own"`), `felucca-bsp`, `optimist` (w0ts, `fcdc488`), `fm1_open` (M-VAVE).

## 0. Short answer

- **It is feasible, and the code already exists.** One open, clean-room, bare-metal BLE stack serves every
  non-JieLi firmware on the FM-1: **Optimist's** (Henri David / hdavid). It has a link layer, L2CAP/ATT/GATT, SMP,
  BLE-MIDI and the WL82 baseband driver, with no tasks, no heap and no JieLi library. Melodee ships it in its
  1.6.0 RCs, fm1-base links it with `ble_stack = "own"`, and fble2 wraps it as an fext module. JIANT's interrupt
  plan (ALNK0 at 3, TIMER5 at 4, core 1 only inside the audio ISR) is the same as Melodee's, so Melodee's port is
  the closest template.
- **RSSI is in the hardware for every received packet**: the engine writes the `RSSI0-3` words at control-block
  offset 0x138. The stack already reads them for scan reports. It does not read them for connection packets yet,
  but that takes about 3 lines in the RX ISR. A connected phone or computer gives one sample per connection event,
  about **89–133 per second**. The value is a raw AGC/gain word. Turning it into dBm needs a 64-entry gain table
  that nobody has transcribed, so the output is **relative**. That is enough for a proximity source that calibrates
  itself.
- **Flash is the problem, not RAM.** The app slot has about 2.5 KB free. The stack needs about 36 KB as a
  peripheral with live calibration, or about 48 KB with the scanner. Linking it straight into the app (option A)
  means freeing about 45 KB.
- **The recommended route is C2, an "extension segment".** The stack is linked *with* JIANT, so there is no ABI.
  Its code and tables sit at a fixed address in the free flash at **0xEA000–0xFBFFF (72 KiB, unused by JIANT)**
  and run from the plain XIP window, the way fble modules already run. The app pays about 1–2 KB, everything stays
  GPL-3.0-only, and nothing has to be deleted. Option A is the fallback if a second region proves impractical.
  Option B (fext + fble2) adds a socket JIANT does not have, an unfinished ABI 1.1 and an untested module.
- **Calibration**: JIANT never writes 0xE8000/0xE9000 from the app. But it does keep the FM6 voice bank at
  **0xE7000** (a VM *candidate*). Worse, **its update loader's record sweep can erase 0xE8000**, the sector where
  stock V15 keeps its VM and also its update record (0xE8F00). Assume any unit that went stock → Felucca/JIANT has
  lost its trims. Use **live calibration (`fm1_rf_cal.h`)** as the default and treat VM trims as a bonus.

## 1. How SLOOP and Melodee do BLE

### 1.1 SLOOP

- **Upstream SLOOP (isod89) has no BLE.** `sloop-fm1/README.md:372` and `SLOOP.md:577`: "Bluetooth MIDI is not
  supported: SLOOP, like Felucca, never switches the radio on." No branch or PR head has BLE code (every
  `refs/pull/*/head` was grepped for `ble_hw|fext_host|bluetooth midi`, with no hit).
- **The SLOOP BLE that exists is jmarianski's**: the `fext` branch of `jmarianski/sloop-fm1` adds a plugin socket,
  and the **fble** module supplies Bluetooth through it.
  - Socket: `sloop-fext/firmware/src/fext_host.c` (774 lines), `fext.h`, `fext_load.h`, `firmware/app_fext.ld.in`.
  - Slot: **USR4 0xE7000, 80 KiB** (`fext_host.h:4-11,15-16`). This **covers the VM and BTIF**.
  - RAM windows: CODE 4 KiB at 0x01C06F00, DMA 24 KiB at 0x01C08000, RAM 8 KiB at 0x01C1E000.
  - NV sector: 0xFB000.
  - Interrupts: the module's JieLi priorities 0..2 are mapped to **5..7, above audio** (`fext_host.c:206-218`).
  - `quiet()` masks ALNK0 and TIMER5 while the radio calibrates, ending after at most 3 s (`fext_host.c:225-242,
    458-460`).
  - The host also switches on the stock DC-DC (`main.c:395`, `hal/fm1_power.h:24-36`). Without it, "the radio's
    calibration draws a current that, on the boot loader's linear 1.3 V, faults the bus at random".
  - **fble 1** (works on hardware): about ¾ of it is JieLi's binary `btctrler.a`/`btstack.a`, which runs as tasks
    through fm1-base's OS shim. It needs the 80 KiB slot. **Installing it erases the RF calibration** at
    0x0E7000–0x0E9FFF (`fble/README.md:27-33`).
  - **fble 2** (prototype, never run on an FM-1): Optimist's stack plus felucca-bsp's radio headers, MPL-2.0. Images
    are 34.3 KB (peripheral c0), 45.8 KB (+ central, no pairing, c3) and 53.9 KB (+ pairing, c1). DMA RAM is
    5.8–7.9 KB, CODE 36 B. It includes live calibration (+5.2 KB). The proposed slot is **0xEA000**
    (`fble/docs/FBLE2-PLAN.md:13-21,113-145`).
  - Open blockers listed there: ABI 1.1 (`irq_attach2` below audio), hardware bring-up, the radio start-up with
    CPU1/XIP running, and NV writes blocking audio (`FBLE2-PLAN.md:271-287`).

### 1.2 Melodee (keremimo/melodee, v1.6.0-rc.1 … rc.7)

- **Stack**: Optimist's, imported from `w0ts/optimist@498a3ae`. It lives in `firmware/src/ble/` (33 files) plus
  `firmware/hal/fm1_ble.h`, `fm1_ble_rf.h`, `fm1_rf_cal.h` and `ble_rf_tables.h` (`docs/BLE_MIDI.md:110-127`).
  - It is on by default (`firmware/src/melodee.c:38-40`, `MELODEE_BLE 1`) and built as a unity build
    (`src/midi_ble.c:7` includes `ble/ble_stack.c`).
  - Features: one link, as peripheral or central, with LE legacy pairing (Just Works or passkey) and a 27-octet LL
    PDU (`BLE_MIDI.md:3-6,46-62`).
- **License**: the files carry `GPL-3.0-only` (Melodee relabelled them). Upstream they are
  `GPL-3.0-only OR MPL-2.0` (`optimist/LICENSING.md:62-70`).
  - `fm1_rf_cal.h` is Apache-2.0: JieLi plus Charles Vestal, rewritten in C from the SDK's `wl_rf_common.a`.
  - `ble_rf_tables.h` holds V15-captured register data. The felucca-bsp copy is marked Apache-2.0, on the grounds
    that M-VAVE published V15 under Apache-2.0 in fm1_open. Melodee's own copy notes "no stated upstream licence"
    (`BLE_MIDI.md:120-124`).
- **Cost**: Melodee publishes no BLE-only delta. Its image is 532,136 B of a 573,440 B XIP slot; RAM is 97,016 of
  98,304 B; the pool is 162,112 of 344,064 B (`build-rel.log:27`). Optimist's measured deltas, which Melodee's
  configuration matches:

  | Optimist builder item (`optimist/tools/builder/costs.json`) | Flash | RAM |
  |---|---|---|
  | BLE (peripheral, BLE-MIDI) | 30,904 B | 6,476 B |
  | + BLE_CENTRAL=3 (scanner + initiator, no pairing) | 12,428 B | 1,520 B |
  | + BLE_CENTRAL=1 (with pairing / SMP initiator) | 20,852 B | 2,224 B |
  | + BLE_BOND | 3,916 B | 464 B |
  | + live calibration `fm1_rf_cal.h` (FBLE2-PLAN §6) | ≈ 5,200 B | ≈ 240 B |

  So Melodee spends roughly 55–60 KB of flash, part of it outside the app slot (see the resource region below),
  and about 9 KB of RAM.
  - **DMA RAM** is one static **1,508 B baseband block**, which the engine reads and writes by DMA
    (`docs/BLE-HW-FACTS.md` §4). Melodee places it **in the pool**: `src/midi_ble.h:10-11` defines
    `BLE_DMA_MEMORY` and `FM1_RF_CAL_MEMORY` as `.pool`, starting at 0x01C20000.
  - Melodee also moves the **radio start-up program (5.8 KB packed) out of the app slot**, into a separately
    written "resource region" at 0xCB000 that is read through the plain XIP window (`src/res.c:1-11`; `RES_OK()`
    gates the radio in `src/midi_ble.c:147`).
- **Task/IRQ model**: there are no tasks. Everything runs in **IRQ 45 (event) and IRQ 29 (RX) at priority 2 on
  CPU0**, below audio (ALNK0 = 3) and TIMER5 (4) (`src/ble/ble_hw_wl82.c:23,48-49,1198-1206`;
  `hal/fm1_ble.h:319-323`). The main loop talks to the stack through single-producer rings.
  - The baseband engine meets every radio deadline in hardware. The software deadlines are ≥ 7.5 ms, so "BLE waits
    for an audio half and the audio never waits for more than one BLE handler" (`docs/BLE-STACK.md` §7, line 157).
  - The handlers run **from flash**. Every flash write disables all IRQs, and the engine runs alone meanwhile
    (missed events become the central's retransmissions).
- **CPU1 / audio / XIP during radio bursts**: the radio is started only on demand, never at boot. It needs stopped
  music, released voices and idle USB audio (`BLE_MIDI.md:12-15`; `src/ui_ble.c:11`).
  - `mel_ble_radio_start` (`src/main.c:208-215`) turns IRQs off, silences the audio buffer and runs
    `ble_hw_wl82_start`. CPU1 stays in its RAM idle loop.
  - The **crystal ramp (group 5) runs from `.ram_text` with IRQs off** (`hal/fm1_ble_rf.h:305-307`). Without that,
    the FM-1 hung when a flash fetch landed mid-ramp (`BLE-STACK.md` §8.1, lines 181-183).
  - `tools/build.py:397-401` refuses a build where the ramp is not in RAM.
- **RF calibration**: VM records 106/107/108/187.
  - The reader tries 0x0E8000 (measured on hardware), then 0x0E7000 (candidate), then 0x093000/0x095000 (emulator)
    (`src/ble/ble_vm.h:9-34`). It requires both the `55 AA AA 55` magic and a valid first record, and it **never
    writes**.
  - A 100-byte CRC-protected copy goes into Melodee's own settings record (`BLE_MIDI.md:64-77,96-103`).
  - With no valid set, it falls back to **live calibration**: the SDK board constants for 106–108, and 187 measured
    in about 160 ms (`src/midi_ble.c:122-143`). This was checked on an FM-1 whose VM had been overwritten.
  - Melodee itself still lists 0xE7000, 0xE8000 and 0xE9000 among its preset-pool sectors
    (`src/storage.c:105-107`). That is why it relies on live calibration.
- **RSSI exposed?** Only for scan reports: the device list shows 1–3 relative bars (`src/ble/ble_scan.c:213-244`).
  Nothing is exposed for a connected link.

### 1.3 fm1-base (charlesvestal), for completeness

- `ble_stack = "own"` links Optimist's `bt/ble` (dual-licensed) and the felucca-bsp radio. It calibrates at every
  start (`README.md:59-65`; `device/bt_own.c:1-12,225`).
- It was tried on one FM-1 with a Mac and an iPhone. It costs **+41–46 KB flash, +10 KB RAM, +1 KB pool**
  (`README.md:138`).
- The JieLi route costs +124 KB flash and +45 KB RAM, which is out of the question for JIANT.

## 2. RSSI per packet

| Path | Where the stack reads it | Rate | Status |
|---|---|---|---|
| Scan reports (adverts, no connection) | `ble_hw_wl82_central.c:137` `rssi = CB->rssi[2]` → `ble_ll_hw_adv_report` → ring `ble_ll_scan_take(pdu, &rssi)` (`ble_ll_central.c:46-82`) | each advert heard while scanning; window 37.5 ms / interval 40 ms; depends on the peer's advertising interval (20 ms … >1 s) | **exists** (needs `BLE_CENTRAL`=3, +12.4 KB) |
| Connection packets (FM-1 peripheral or central) | not read today; the field is mapped (`ble_hw_wl82.c:106` `volatile uint16_t rssi[4]; /* 0x138 */`); insertion point: `hw_rx_service`, after the good-packet branch, before re-arming (`ble_hw_wl82.c:964-984`) | **one per connection event**, empty PDUs included: 133 Hz @ 7.5 ms, 89 Hz @ 11.25 ms (the intervals the peripheral asks for, `BLE-STACK.md` §5) | **3-line change** + a ring / atomic word |
| While only advertising | only SCAN_REQ / CONNECT_IND are received | – | not useful |

- **Units**: a raw 16-bit word. Per `BLE-HW-FACTS.md:152`, dBm = 21 − (w & 63) − T[(w >> 8) & 63], where T is a
  64-entry gain table that has **not been transcribed** (open item U9, `BLE-HW-FACTS.md:349`). The stack's stand-in
  is `ble_scan_strength(w) = 255 − ((w & 63) + ((w >> 8) & 63))`, which assumes larger means weaker [inferred]
  (`ble_scan.c:213-218`).
- Which of RSSI2 / RSSI3 belongs to which RX buffer in a connection is unverified. HW §8 says the vendor reads "RSSI2
  / 3" in IRQ 29.
- **Proximity derivation** (proposed; it runs in the main loop or TIMER5, never in the BLE ISR beyond storing the
  word):
  1. The ISR stores `{word, LASTCHMAP channel, time}` for CRC-good packets only, into a 16-entry ring.
  2. Map the word to a level `x` using a measured T if available, else the stand-in metric. **Check monotonicity
     across AGC steps on hardware first.**
  3. Remove the per-channel offset. Hopping across 37 channels gives several dB of channel-dependent fading, so keep
     an EMA of `x` per channel index and subtract it from a global mean.
  4. Apply a median of 3, then an attack/release one-pole (e.g. 20–40 ms attack, 150–300 ms release).
  5. Normalise to 0..1 with auto-ranging or a user CAL: "hand away" (max) and "hand on the antenna" (min). Apply a
     dead zone and a curve.
  6. On a gap over 100 ms (a flash write, link loss), hold the value; ease to "far" after the link drops.
  7. Output Q15 0..1 as a new mod source `MS_PROX` next to `MS_MODW` (`firmware/src/mod.c:39`). Each track's slots
     already read sources once per block.
- **Best source**: a phone or computer **connected over BLE-MIDI**. The rate is high and the peer is known, and the
  hand attenuates its packets at the FM-1's antenna. A scan-only mode (a chosen advertiser's address, or simply the
  strongest one) is possible with the scanner, but its rate and address stability (random addresses rotate) are
  poor. Treat it as a secondary mode.
- The stack allows one link at a time, and scanning pauses advertising. Proximity and BLE-MIDI share the same link.

## 3. JIANT's flash map vs the calibration area

Sources: `firmware/app.ld:17-30`, `tools/fm1pkg_make.py:29-35,141-146`, `firmware/hal/fm1_flash.h:37-57`,
`firmware/src/storage.c:20-35,70-85`, `firmware/src/fm6_vbank.c:11-26`, `firmware/loader/loader.c:20-21`,
`firmware/loader/ldr_core.c:31-36,171-179`.

| Flash | Use in JIANT | Notes |
|---|---|---|
| 0x000000–0x003FFF | head (SPL, isd_config) | never written |
| 0x004000–0x092FFF | app area: app.bin 0x8EA90 = 584,336 B slot (XIP 0x02000120), cfg_tool, eq | image 581,752 B → **2,584 B free** |
| 0x093000–0x096FFF | unused (the stock package's declared "VM" region starts here) | 16 KiB free; JIANT app cannot write it (FL_RANGE_OK) |
| 0x097000–0x09EFFF | 4 project slots, A/B | |
| 0x09F000 | user presets' FM6 patches, copy A | |
| 0x0A0000–0x0D7FFF | songs 2..8 (4 objects × A/B) | |
| 0x0D8000–0x0D9FFF | song index A/B | |
| 0x0DA000–0x0DBFFF | **free** (storage.c:31-32) | 8 KiB |
| 0x0DC000–0x0DFFFF | user preset banks | |
| 0x0E0000–0x0E4FFF | OTA loader staging; JIANT's update record at 0xE4F00 | |
| 0x0E5000–0x0E6FFF | autosave A/B (also stages a new FM6 bank) | |
| **0x0E7000** | **FM6 voice bank** (written only when a bank is uploaded) | **VM candidate (2nd area)**: on the one measured unit it read FF |
| **0x0E8000** | not written by the app | **stock VM: RF trims 106/107/108/187** (measured; `ble_vm.h:9-12`) |
| **0x0E9000** | not written by the app | **BTIF** (classic BT address, record 102) |
| **0x0EA000–0x0FBFFF** | **unused** (the package declares it "USR", fm1pkg_make.py:146) | **72 KiB free: the module / extension slot** |
| 0x0FC000–0x0FDFFF | settings A/B | |
| 0x0FE000 | user presets' FM6 patches, copy B | |
| 0x0FF000 | key_mac | factory |

**Does JIANT already overwrite the calibration?**

1. **The app: no for 0xE8000/0xE9000.** `FL_STORE_OK` stops at `FL_VBANK_HI` = 0xE8000 (`fm1_flash.h:51-57`).
   **Yes for 0xE7000** (the FM6 voice bank, `fm6_vbank.c:26`). That sector is only a VM candidate, so it probably
   does not hold the trims, but the conservative rule (fble, FBLE2-PLAN §5) treats 0xE7000–0xE9FFF as factory. The
   bank's sector starts with JIANT's "FELU" commit record, so the VM reader (which checks magic plus record CRC)
   cannot mistake it for a VM.
2. **The update loader: probably yes, once.**
   - `loader.c:21` allows writes in 0xE0000–0xFC000.
   - After an install, `ldr_core.c:171-179` scans every 4 KiB sector of [0x93000, 0xFC000) outside 0x97000–0xDFFFF.
     It **erases** any sector whose last 256 bytes hold a valid update record (`0x5441` + CRC16).
   - The loader's own comment says the stock firmware's record is at **0xE8F00** (`ldr_core.c:35`). That is inside
     the VM sector.
   - So an install from stock V15 through M-UPGRADE would erase 0xE8000, VM trims included.
   - Optimist's loader explicitly skips 0xE7000–0xE9FFF, and it measured the VM surviving stock → Optimist
     (`BLE-HW-FACTS.md` §15, lines 413-419). Felucca's and JIANT's loaders do not skip it (felucca-bsp's
     `loader/ldr_core.c` and Melodee's are the same).
   - This is not measured on a JIANT unit. **Verify by reading 0xE8000 before any radio code runs.**
3. Other things that may already have erased it on a given unit: SLOOP 2.4 USR4 samples, fble 1, older Melodee
   preset writes, or a stock restore that re-formats the VM.

**Free flash for a module**: yes. 0xEA000–0xFBFFF (72 KiB) is free and is the slot fble2 proposes (FBLE2-PLAN §5).
fble2 c0, c3 and c1 (34–54 KB) and a C2 extension segment (36–52 KB) all fit. One constraint: the loader's sweep
would erase any sector whose 4K−256 offset parses as an update record. The segment's builder must check that, the
way `fm1pkg_make.py:155-157` does for the app.

## 4. RAM/DMA and the options

**JIANT today** (from `build/felucca.elf` symbols):

| Region | Range | Used to | Free |
|---|---|---|---|
| RAM .data+.bss | 0x01C08000–0x01C20000 | 0x01C1ED4C | **4,788 B** |
| LOWBUF | 0x01C00000–0x01C08000 | 0x01C07C58 | 936 B |
| POOL (+ core-1 stacks) | 0x01C20000–0x01C6D800 | 0x01C6A580 | **12,928 B** |
| RAMTEXT (.ram_text, .c1_text, .hot_text) | 0x01C6D800–0x01C76000 | 0x01C7560C | 2,548 B |

**What the stack needs**:

- About 6.5 KB of RAM for the peripheral, +1.5 KB for the scanner (`costs.json`).
- 36 B of RAM code (`fm1_rf_burst_run`).
- The 1,508 B baseband block in DMA-reachable SRAM.
- No heap and no task stacks. It runs on the ISR stack: JIANT's supervisor stack is 0x01C7A100–0x01C7C000.

**DMA reach** has conflicting notes:

- Felucca-fext says "a radio's DMA reaches only low RAM (0x01C08000 does, 0x01C20000 does not)"
  (`felucca-fext/firmware/src/fext_host.h:12-14`). That observation was probably made with JieLi's libraries.
- Melodee puts the block in `.pool` at ≥ 0x01C20000 and reports MIDI working on hardware.
- The block's pointers are 16-bit offsets from a programmable base (`BLE-HW-FACTS.md` §3-4).
- JIANT's PRP write-limit window 3 (`_rt_start`…`_sstack_top`, `fm1_guard.h:16-24`) blocks every DMA there, so the
  block must not sit there either.
- **Plan**: the block in the pool, everything else either in the pool or in .bss, which fits (4.8 + 12.9 KB). Test
  pool placement first, and keep a .bss fallback ready.

**Options**:

| | (A) Optimist linked into the app | (B) fext socket + fble2 module | (C2) extension segment (recommended) |
|---|---|---|---|
| Where the stack lives | app slot | own slot 0xEA000, relocatable image | 0xEA000, **linked with the app at a fixed address**, runs from the plain XIP window |
| App flash | +36 KB (peripheral + live cal) … +48 KB (+ scanner) +2–4 KB glue/UI; only 2.5 KB free → **free ≈ 40–50 KB** | the socket: `fext_host.c` is ≈ 770 lines with UI pages and the SysEx push protocol (est. 5–8 KB) + ABI 1.1 (≈ 1 KB). Still more than 2.5 KB | ≈ 1–2 KB (ISR wrappers, glue, a build-ID/CRC check of the segment, PC-guard widening) |
| RAM | ≈ 6.5–8 KB (bb in pool) | DMA window 5.8–7.9 KB as fble2 is linked today (all module data in DMA) + 36 B CODE | as A |
| Interface | direct calls | ABI 1.0/1.1 (1.0 attaches IRQs above audio, risk R1); RSSI/mod source not in the ABI (needs an extension) | direct calls (one build, one link map) |
| Install | normal update | separate `fext_push` | the installer/editor writes a second region (Melodee's resource region shows how) |
| Hardware maturity | Melodee/fm1-base: working | fble2 never run on an FM-1 | same code as A; plain-XIP execution is proven by fble 1 modules |
| Licence | GPL-only or dual files in a GPL work | MPL module beside a GPL host | as A |

**If (A) is chosen**, the biggest candidates to free or move (`nm -S --size-sort build/felucca.elf`, flash
symbols):

| Candidate | Size | How |
|---|---|---|
| VOICE "sing" lyrics: `LY_EN_DICT` 13,346, `LY_ES_TXT` 1,960, `LY_EN_TXT` 1,561, `LY_JA_TXT` 1,029, `LY_EN_IDX` 524, `PHON` 644 + code (`sy_word` 3,906, `sy_tick`, `ly_keyname`…) | ≈ 19 KB data, ≈ 33 KB with code | move the tables to a resource region (no feature loss) |
| Song generator GEN (`gen.c`, `gen_tables.c`: `gen_role` 9,710, `gen_make` 9,680, `gen_drums` 2,844, `gen_post` 2,256, `GENRE` 2,236, `gen_pick`…) | ≈ 30 KB | remove, or make a build option |
| Anti-aliased fonts `AF_M/L/S_DATA`, `KC_DATA`, `AF_M_KP` | ≈ 14 KB | resource region |
| BLE radio program `ble_rf_prog` (would be added) | 5.8 KB | resource region from day one (as Melodee) |
| Dice (`part_dice`, `song_dice`), `fm4_convert` (FM4 import 2.5 KB), `splash_sig` 1.5 KB | ≈ 8 KB | build options |
| UI giants `draw_graph` 28,972, `ui_input_frame` 19,526, `fm1_main` 14,424, `ed_handle` 10,272, `draw_columns` 10,254 | — | size work only (already -Os); not realistic quick wins |

Option A also shifts the pinned sound layout (`tools/audio_layout.txt`, `FELUCCA_REPIN`). The new code goes after the
pinned runs, so a repin may be needed.

## 5. Licensing (JIANT is GPL-3.0-only, `LICENSING.md`)

| Piece | Licence | With JIANT |
|---|---|---|
| Optimist stack `ble/*` (upstream, fm1-base `bt/ble`, fble2 copy) | GPL-3.0-only OR MPL-2.0 | ✔ use the GPL option (or MPL; MPL-2.0 is GPL-compatible, no Exhibit B) |
| Melodee's copy of the same files and its glue | GPL-3.0-only | ✔ |
| `fm1_ble.h`, `fm1_ble_rf.h` (felucca-bsp / Optimist) | GPL-3.0-only OR MPL-2.0 | ✔ |
| `fm1_rf_cal.h` | Apache-2.0 (JieLi + C. Vestal; C rewrite of the SDK's binary `wl_rf_common.a`) | ✔ Apache-2.0 → GPL-3.0 is one-way compatible (JIANT already ships Apache-2.0 `fm6_core.c` and SDK files). Caveat: the SDK's Apache-2.0 claim for binary libs is "not confirmed with JieLi" (felucca-bsp `LICENSING.md:54`) |
| `ble_rf_tables_v15.h` (register writes captured from V15) | Apache-2.0 per felucca-bsp (M-VAVE published V15 under Apache-2.0 in fm1_open `THIRD_PARTY.md:39`); Melodee: "no stated upstream licence" for the data | ✔ take felucca-bsp's Apache-2.0 copy; keep NOTICE |
| (A) / (C2) | the above, all in one GPL-3.0-only work | ✔ |
| (B) fble2 module | MPL-2.0 + Apache-2.0, a separate work loaded at runtime | ✔ (the host stays GPL; the socket files are MPL) |
| fble 1 (JieLi `btctrler.a`, `btstack.a`, binary only) | "Apache-2.0", no source | ✘ not in a GPL work; only beside it; avoid |
| fm1_open SDK (`libfm1_engine.a`, proprietary, FM1-only grant; GPL exception only for M-VAVE's APP) | proprietary | ✘ incompatible with Felucca-derived GPL code (no linking exception from Hügelton et al.) |

## 6. Recommended plan

**Route: C2 (extension segment) with Optimist's stack. Take the stack files from the dual-licensed upstream
(fm1-base `bt/ble`, the hardware-tested version with the UBSan fix) and take the integration patterns from
Melodee: pool placement, the iterative RF replay, quiet start, the VM reader, the settings copy.** Peripheral first;
the scanner (`BLE_CENTRAL=3`) later for scan-mode proximity and connecting out.

**Stage 2 steps (each separately reviewable):**

1. **Read-only probe (no radio)**. On the user's own unit, take a full 1 MiB backup from UBOOT (m-flasher /
   `fm1_rescue.py` without `--write`). Then dump 0xE7000–0xE9FFF through a console `flr` command and decode it
   (Optimist `tools/ble_vm.py`). This settles whether the trims survived. (½–1 day)
2. **Safety fixes before any radio work**:
   - the loader sweep skips 0xE7000–0xE9FFF (as Optimist does);
   - move the FM6 voice bank from 0xE7000 to the free 0xDA000 (with a one-time copy);
   - reserve 0xEA000–0xFBFFF as the extension slot.

   (1–2 days)
3. **Power/clock prerequisites**:
   - Add the stock DC-DC switch-on: `fm1_power_stock`, as SLOOP-fext and Melodee's `fm1_power_init` do. JIANT has
     only `fm1_rails_boot` (`hal/fm1_sys.h:119-133`), without the DC-DC step.
   - Confirm TIMER4 runs at 24 MHz under both CPU CLOCK settings (360/320 MHz).

   (1 day)
4. **C2 plumbing**: a linker region at XIP 0x020E6000 (flash 0xEA000) for `.bt_text`/`.bt_rodata`; widen the PC
   limit (`fm1_guard.h:28`, today `[0x02000120, _etext]`); a header with build-ID and CRC that the app checks before
   enabling BLE (as `res.c` `res_valid`); a writer through the editor protocol; a record-lookalike check. (3–5 days)
5. **Port the peripheral** (BLE-MIDI in/out into JIANT's MIDI router, IRQ 45/29 at priority 2, the radio started
   only on demand with audio quiet, live calibration by default, VM trims when valid, the address and a 100 B trim
   copy in settings). (1–2 weeks)
6. **RSSI tap + `MS_PROX`**: store the RSSI word per good packet in `hw_rx_service`, then build the filter chain of
   §2, a CAL page and a live meter. (3–5 days)
7. **Scanner mode** (optional, +12.4 KB, fits the 72 KiB segment). (3–5 days)

**Total ≈ 4–6 weeks part-time** to a usable peripheral with the proximity source (in line with FBLE2-PLAN's own
2–4 weeks plus JIANT-specific flash work).

**Risks:**

- **Bricking / calibration loss.** The radio start-up can hang. Use a boot guard: never auto-start the radio in the
  first boot after a crash, keep "OFF" the default, and add a breadcrumb in `.noinit` (Optimist's
  `prev_ble`). Never write 0xE7000–0xE9FFF. Back up before testing. Live calibration removes the dependence on
  trims, but its constants were validated on few units (R7).
- **Audio glitches.** The 160 ms calibration and the crystal ramp run with IRQs off, so allow the start only while
  stopped, then fade in. The BLE ISRs below audio wait for at most one 2.9 ms half (`core.h:15`), which is fine for
  ≥ 7.5 ms deadlines. They also run from XIP and **compete for the I-cache with the render** (JIANT pins its sound
  code for exactly this reason). Software AES (only if pairing) costs up to 35 AES blocks per PDU.
- **XIP stalls.** Flash writes (autosave, saves) disable IRQs. The engine keeps the link alive, and RSSI samples get
  a gap (hold the value). Keep the ramp code in `.ram_text` (build check). CPU1 is idle in RAM outside the audio
  ISR, so it is safe while IRQs are off.
- **DMA placement** (pool vs low RAM, see §4) and **DC-DC** prerequisites.
- **RSSI semantics**: the gain table is unknown, AGC steps may be non-monotonic, multipath and channel hopping add
  noise, and the hand effect depends on where the antenna is. All of this must be measured before promising a
  musical range.

**Test on hardware first, in this order:**

1. VM dump (read-only).
2. The DC-DC step alone, then audio stability.
3. Radio start with live calibration, audio stopped, on 360 and 320 MHz.
4. Advertising + Mac/iPhone connect, BLE-MIDI both ways.
5. Audio under a connection: block overruns and peak ticks with the heaviest project, core 1 on and off.
6. Autosave and project save during a connection (link survives, no audio dropout).
7. A raw-RSSI log over the console at fixed distances and with a hand on/off the antenna (monotonicity, noise,
   per-channel spread).
8. Only then, the filter and mod source.

## 7. As built: stage 2 step 3 (V10.12, branch codex/add15-ble)

Route C2, peripheral only, MIDI not routed yet.

- **Segment.** `firmware/bt/bt_seg.c` is a unit of its own (Optimist's stack from fm1-base `bt/ble`, Melodee's
  `hal/fm1_ble.h` / `fm1_ble_rf.h`, felucca-bsp's `fm1_rf_cal.h` / `ble_rf_tables_v15.h`). `app.ld` links it at XIP
  0x020E6000 = flash 0xEA000 (`BTSEG`, 72 KiB): a 32-byte header (`JBT1`, payload length, CRC-32), then its code and
  constants (≈ 27.3 KB, 7 sectors). Its RAM is in the pool, its 1,508 B baseband block in `.bss` (low RAM, which every
  DMA reaches), its ramp burst (36 B) in `.ram_text` (build check). No initialised data (build check).
- **App side.** `src/bt.c` (MENU > SYSTEM > BLUETOOTH, the settings bytes, the boot guard) and `src/bt_hw.c` (the
  segment check, the start with the interrupts off and the audio buffer silent). `tools/build.py` writes the segment's
  length and CRC-32 into the app's `bt_want`; the app calls nothing in the segment unless the header and the CRC-32 of
  the payload as read through XIP match. `fl_plain_window_init` is set before the first read (JIANT never set it before).
  The PC guard's window reaches `_bt_end`.
- **Install.** The package gets a third UFW entry, `bt.bin` (type 200), after `ota.bin`. JIANT's update loader checks
  it with the rest before erasing anything (entry CRC16, header magic, per-sector CRC16) and writes it after the app
  area, sector by sector with verify; the last sector's tail is 0xFF. A package without it leaves the region alone.
  The record sweep already skips 0xEA000–0xFBFFF. `tests/ldr_test.c` covers it.
- **Start.** IRQ 45 / 29 at priority 2, `fm1_rf_cal_on` with the SDK board constants for 106–108 (187 measured), then
  `ble_init` and advertising as "JIANT FM-1" with the BLE-MIDI service UUID. Only while stopped; OFF stops advertising
  and closes a link (the radio stays set up until power-off).
- **Boot guard.** `bt_crumb` (`.noinit.zbt`, after every other `.noinit`): set for the start, `RUN` after it. A reset
  during the start, or a crash / watchdog reset while it ran, turns BLUETOOTH OFF at the next power-on and says so.

## 8. As built: stage 2 step 5 (V10.13)

BLE-MIDI both ways.

- **In.** The stack decodes each write to the MIDI characteristic (`ble_midi.c`: running status, several messages a
  packet, real time anywhere, SysEx over packets). `bt_seg.c ble_app_midi_in` keeps what USB's `midi_in_event` keeps
  (`firmware/bt/bt_midi.h bt_midi_accept`: channel messages, F8 FA FB FC; SysEx and the other system messages dropped:
  the editor and the updater stay on USB) and calls the app's `bt_midi_rx` (`src/bt.c`), which puts it into the one MIDI
  input ring (`usb.c midi_enqueue`, source 1 = USB) with the interrupts off: the BLE interrupts (priority 2) are
  preempted by TIMER5, the ring's other producer (USB, TRS), and core 1 takes no interrupt. The audio interrupt drains it
  as before, so notes, CC, PC, bend, aftertouch, CLK USB (the clock and START / CONTINUE / STOP), LEARN, ROUT and MIDI MAP
  treat Bluetooth as USB. A full ring is USB's overflow (every note let go). Received timestamps are not used. No change
  to the sound's code for this.
- **Out.** `usb.c midi_out_event` (the audio interrupt, every message MENU > CONTROL > MIDI OUT lets out: KEYS / CLOCK /
  ALL / OFF) also puts it into `bt_oq` (64 entries) while a central has the MIDI notifications on (`bt_oq.on`, set by
  `ble_app_state`; a new listener starts empty). The stack drains it each connection event (`ble_att.c att_midi_out`:
  BLE-MIDI packets of MTU - 3 octets, header and timestamps, running status), stamped with the segment's own millisecond
  count (TIMER4). Nobody listening, a full ring: dropped. The sound's code moved by 64 B (`tools/audio_layout.txt`
  repinned).
- **Flash.** The app had 128 B free. The app's own cold constants can now live in the segment: `BTAPP_RO` / `BTAPP_FN`
  (`src/bt.c`, `app.ld` `.btapp.*` after the segment's code, inside its CRC-32). MENU > ABOUT's THE PROJECT, CREDITS and
  LICENSES texts moved there (`ui_menu.c ABOUT_PROJECT`, `ABOUT_CREDITS`: kind octet + text lines). The segment is
  checked once at power-on (`bt_boot`, after the splash: the plain XIP window set, the header and the CRC-32); without it
  (NO SEG) ABOUT shows its header, AUDIO LOAD and the crash lines, no credits. `bt_midi_rx` lives there too (only the
  segment calls it). App 582,140 B (2,196 B free), RAM +284 B (`bt_oq`), segment 30,660 B (8 sectors).
- **MENU tiles.** SYSTEM's eighth row (ABOUT) does not fit a bottom tile of the overview drawn while sliding between
  tabs (step 3 made SYSTEM eight rows; `ui_render` lint): the tile shows seven.
- **Tests.** `tests/ble_midi_test.c` (fm1-base's codec test), `tests/bt_midi_test.c` (packets as apps send them into
  the ring, the out ring and its packets), `tests/ui_test.c` (ABOUT's lines).
