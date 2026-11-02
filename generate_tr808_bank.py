#!/usr/bin/env python3
"""Build a Betty BRR sample bank, 808-dominant, using CC0 TR-808 recordings.

TR-808 source: Michael Fischer (1994), redistributed CC0 by tidalcycles:
https://github.com/tidalcycles/sounds-tr808-fischer
TR-909 and TR-606 accents are ORIGINAL synthetic approximations, not Roland recordings.
"""
import io
import math
import random
import struct
import urllib.request
import wave
from pathlib import Path

FS = 16000
TAU = 2 * math.pi
BASE = "https://raw.githubusercontent.com/tidalcycles/sounds-tr808-fischer/main/"
SOURCES = {
    "dataKick808": ("bd8/BD5050.WAV", .43),
    "dataSnare808": ("sd8/SD5050.WAV", .38),
    "dataClap808": ("cp8/CP.WAV", .38),
    "dataCH808": ("ch8/CH.WAV", .20),
    "dataOH808": ("oh8/OH50.WAV", .44),
    "dataTom808": ("lt8/LT50.WAV", .34),
    "dataRim808": ("rs8/RS.WAV", .25),
    "dataCow808": ("cb8/CB.WAV", .25),
}
ORDER = [
    ("dataKick808", 1.20), ("dataKick909", 1.22), ("dataSnare808", 1.75), ("dataClap808", 1.90),
    ("dataCH808", 1.75), ("dataOH808", 2.15), ("dataCH606", 1.95), ("dataOH606", 2.10),
    ("dataTom808", 1.45), ("dataRim808", 1.65), ("dataSnare909", 1.70), ("dataCow808", 1.55),
]
MAX_SECS = .58

def decode_wav(blob, max_s):
    with wave.open(io.BytesIO(blob), "rb") as wav:
        channels, sampwidth, sr = wav.getnchannels(), wav.getsampwidth(), wav.getframerate()
        n = min(wav.getnframes(), int(max_s * sr))
        frames = wav.readframes(n)
    if sampwidth not in (1, 2, 3, 4):
        raise ValueError(f"unsupported PCM width {sampwidth}")
    def sample_at(index):
        off = index * sampwidth
        if sampwidth == 1:
            return (frames[off] - 128) / 128
        if sampwidth == 2:
            return struct.unpack_from("<h", frames, off)[0] / 32768
        if sampwidth == 3:
            return int.from_bytes(frames[off:off+3], "little", signed=True) / 8388608
        return struct.unpack_from("<i", frames, off)[0] / 2147483648
    count = len(frames) // (channels * sampwidth)
    source = [
        sum(sample_at(i * channels + channel) for channel in range(channels)) / channels
        for i in range(count)
    ]
    if not source:
        raise ValueError("empty WAV")
    # Pre-filter to minimize aliasing at NTS-1 playback rate.
    if sr > 22000:
        smoothed = []
        for i, val in enumerate(source):
            smoothed.append((source[max(0,i-2)] + 2*source[max(0,i-1)] +
                             3*val + 2*source[min(count-1,i+1)] +
                             source[min(count-1,i+2)]) / 9)
        source = smoothed
    length = min(int(max_s * FS), int(len(source) * FS / sr))
    out = []
    for j in range(length):
        x = j * sr / FS
        idx = min(int(x), len(source)-1)
        a = x - idx
        out.append(source[idx] * (1-a) + source[min(idx+1, len(source)-1)] * a)
    # Trim trailing silence but keep at least 100 ms; modest normalization.
    threshold = .0025
    last = max((i for i, val in enumerate(out) if abs(val) > threshold), default=0)
    keep = min(len(out), max(int(.10 * FS), last+int(.012*FS)))
    out = out[:keep]
    peak = max(abs(x) for x in out) or 1
    gain = min(1.5, .84 / peak)
    for i in range(len(out)):
        fade = min(1, i / 24, (len(out)-1-i)/160)
        out[i] = max(-.9, min(.9, out[i]*gain*fade))
    return out

def generated(name):
    rnd = random.Random(808909606 + len(name))
    duration = {"dataKick909": .30, "dataSnare909": .24,
                "dataCH606": .085, "dataOH606": .31}[name]
    n = int(duration * FS)
    result = []
    phase = 0
    lp = 0
    lp2 = 0
    metal = [0.] * 6
    freqs = [410, 540, 800, 1010, 1330, 1680]
    for i in range(n):
        t = i/FS
        noise = rnd.uniform(-1, 1)
        if name == "dataKick909":
            frequency = 52+170*math.exp(-t/.021)
            phase += TAU*frequency/FS
            tone = (math.sin(phase)+.16*math.sin(2*phase))*math.exp(-t/.145)
            click = .18*noise*math.exp(-t/.004)
            value = .79*tone + click
        elif name == "dataSnare909":
            phase += TAU*181/FS
            lp = .8*lp+.2*noise
            value = .31*math.sin(phase)*math.exp(-t/.055) + .59*(noise-lp)*math.exp(-t/.095)
        else:
            # Filtered six-oscillator metallic noise resembles 606 hats.
            metal_sum = 0
            for k, freq in enumerate(freqs):
                metal[k] += TAU*freq/FS
                metal_sum += (1 if math.sin(metal[k]) >= 0 else -1)
            lp = .91*lp + .09*(metal_sum/6 + .35*noise)
            hp = (metal_sum/6 + .35*noise) - lp
            if name == "dataCH606":
                env = math.exp(-t/.023)
            else:
                env = math.exp(-t/.145)
            value = .57*hp*env
        attack = min(1, t/.001)
        result.append(max(-.85, min(.85, value*attack)))
    # Fade last few samples to prevent click.
    for j in range(min(160,len(result))):
        idx=len(result)-1-j
        result[idx] *= j/160
    return result

def encode_brr(samples):
    # Nintendo/Sony BRR encoder matching Betty's 9-byte, 16-sample blocks.
    # Use filter 0 (independent blocks), or filter 1 (15/16 of previous sample).
    pad = (-len(samples)) % 16
    samples = samples + [0.0] * pad
    data = []
    hist = 0
    for start in range(0, len(samples), 16):
        best = None
        for filt in (0,1):
            for shift in range(13):
                step = 1 << shift
                prev = hist
                err = 0.
                quant = []
                for target_f in samples[start:start+16]:
                    prediction = (prev*15)//16 if filt else 0
                    target = target_f * 32767
                    q = max(-8,min(7,round((target-prediction)/step)))
                    decoded = prediction + q*step
                    err += (target-decoded)**2
                    prev = max(-32768,min(32767,decoded))
                    quant.append(q)
                if best is None or err < best[0]:
                    best = (err, filt, shift, quant, prev)
        _, filt, shift, quant, hist = best
        is_last = start+16 >= len(samples)
        data.append((shift<<4)|(filt<<2)|(1 if is_last else 0))
        data += [((quant[j]&15)<<4)|(quant[j+1]&15) for j in range(0,16,2)]
    return data

def run():
    bank = [
        "#pragma once",
        '#include "userosc.h"',
        "// 8 original TR-808 samples (CC0), plus 4 newly synthesized 909/606-style accents.",
        "struct Sample { uint16_t size; float rate; float divAc; float div; const uint8_t *pData; };",
        "static const uint8_t dataSilence[9] = {1,0,0,0,0,0,0,0,0};",
    ]
    for name, _ in ORDER:
        if name in SOURCES:
            src, duration = SOURCES[name]
            url = BASE + src
            print(f"Fetching {name} from {url}", flush=True)
            req = urllib.request.Request(url, headers={"User-Agent": "Betty-808-builder"})
            blob = urllib.request.urlopen(req, timeout=30).read()
            pcm = decode_wav(blob,duration)
        else:
            pcm=generated(name)
        encoded = encode_brr(pcm)
        bank.append(f"static const uint8_t {name}[{len(encoded)}] = {{")
        for i in range(0,len(encoded),18):
            bank.append("  " + ", ".join(map(str,encoded[i:i+18])) + ",")
        bank.append("};")
        print(f"  {name}: {len(pcm)} samples => {len(encoded)} BRR bytes",flush=True)
    bank += ["// Three unchanged Betty pattern groups; four instrument choices per group.",
             "Sample samples[12] = {"]
    for name, level in ORDER:
        bank.append(f"  {{.size = 0, .rate = {float(FS):.1f}f, .divAc = 32768.0f, .div = 32768.0f * {level:.2f}f, .pData = &{name}[0]}},")
    bank.append("};")
    Path("samplebank.h").write_text("\n".join(bank)+"\n")
    print("Written samplebank.h", flush=True)

if __name__ == "__main__":
    run()
