# BT808 — Betty TR-808 dominant kit (NTS-1 MKI)

Based on the open-source Betty rhythm generator by Jacob Ulmert (BSD 3-Clause).
All original pattern selection, note-triggered sequencer, randomisation, ducking and tuning controls retained.

## Sound bank
| Slot | Sound | Origin |
|---|---|---|
| A1 | TR-808 Kick | Actual CC0 recording |
| A2 | 909-style Kick | New synthetic approximation |
| A3 | TR-808 Snare | Actual CC0 recording |
| A4 | TR-808 Clap | Actual CC0 recording |
| B1 | TR-808 Closed Hat | Actual CC0 recording |
| B2 | TR-808 Open Hat | Actual CC0 recording |
| B3 | 606-style Closed Hat | New synthetic approximation |
| B4 | 606-style Open Hat | New synthetic approximation |
| C1 | TR-808 Low Tom | Actual CC0 recording |
| C2 | TR-808 Rim | Actual CC0 recording |
| C3 | 909-style Snare | New synthetic approximation |
| C4 | TR-808 Cowbell | Actual CC0 recording |

**Eight of twelve sounds use actual TR-808 one-shot recordings**, sourced from Michael Fischer's 1994 TR-808 Sound Sample Set, released under CC0 1.0 through [tidalcycles/sounds-tr808-fischer](https://github.com/tidalcycles/sounds-tr808-fischer).

The two 909-style sounds and two 606-style sounds were synthesized from scratch and are *not* recordings of actual Roland machines. They are labeled accurately to avoid confusion.

## Building

Github Actions runs `generate_tr808_bank.py`, converts all sources to mono 16 kHz BRR data in `samplebank.h`, compiles Betty with the Korg logue SDK, and repacks the instrument into `BT808.ntkdigunit`. The internal directory and manifest use `BT808` for NTS-1 Librarian compatibility.

Original Betty: https://github.com/ulmert/betty
