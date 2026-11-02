# BTMIC — Betty Microhouse (Korg NTS-1 MKI)

A 12-sound original, minimal/microhouse sample kit for Jacob Ulmert's **Betty** oscillator.
The complete original Betty rhythm/pattern engine, accents, randomization, ducking and pitch controls are unchanged.

This sound bank is created from synthesized sine waves, filtered noise and short FM tones (no copyrighted samples).
Every replacement sound is assigned to the **same purpose and exact index** as in Betty's original `samplebank.h`.

## Original Betty slot mapping → Microhouse replacement

| Slot | Original Betty slot | BTMIC replacement | Duration (about) |
|---|---|---|---|
| A1 | Kick (KickJ1) | Sine Kick | 245 ms |
| A2 | Snare (SnareSe2) | Noisy Snare | 150 ms |
| A3 | Rimshot (Rimshot3) | Dry Clap (compact burst) | 64 ms |
| A4 | Percussion (Perc1) | Soft FM Tick | 46 ms |
| B1 | Closed Hat (Closedhat2) | Tight Noisy Hat | 46 ms |
| B2 | Open Hat (Openhat) | Airy Open Hat | 185 ms |
| B3 | Tambourine (Tamb) | Sand Shaker | 81 ms |
| B4 | Percussion (Perc2) | Micro Hat Click | 32 ms |
| C1 | Tom (Tom1) | FM Low Tom | 158 ms |
| C2 | Rimshot (Rimshot2) | FM Woodblock | 71 ms |
| C3 | Triangle (Tri) | Soft FM Plick | 76 ms |
| C4 | Percussion (Perc3) | FM Micro Click | 29 ms |

v1.1: A3 clap now has a compact 10 ms three-transient burst with a 64 ms sample length, versus 27 ms burst / 118 ms length in v1.0. The other eleven sounds are unchanged.

The 16 built-in 16-step Betty patterns heavily feature A1, B1 and B3, with C3/C4 sometimes firing on many consecutive steps. Consequently these latter sounds have short, restrained envelopes.

## Usage

Install `BTMIC.ntkdigunit` with **Korg NTS-1 Digital Librarian**. Use the NTS-1 arpeggiator to clock the Betty patterns. Oscillator parameters A / B / C independently choose 16-step patterns for each group, and Rnd changes random pattern assignments. SHAPE controls ducking and SHIFT+SHAPE tuning, as on original Betty.

## Rebuilding

`generate_microhouse_bank.py` deterministically synthesizes the complete 12-sound bank, converts it to BRR in `samplebank.h`, and emits individual uncompressed WAV previews under `preview_wav/`. GitHub Actions compiles it with the Korg logue SDK and packages it as `BTMIC.ntkdigunit` with matching internal `BTMIC/manifest.json`.

Original [Betty source by Jacob Ulmert](https://github.com/ulmert/betty), BSD 3-Clause.
