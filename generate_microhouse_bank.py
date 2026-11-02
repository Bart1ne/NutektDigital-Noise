#!/usr/bin/env python3
"""Create 12 ORIGINAL minimal-house / microhouse drum sounds for Betty on NTS-1 MKI.

Design: preserve Betty's 3x4 original instrument layout and all original patterns.
All sounds are generated procedurally; no third-party samples are used.
Audio format: mono 16 kHz PCM -> 9-byte / 16-sample BRR blocks.
"""
import math
import random
import wave
import struct
from pathlib import Path

FS=16000
TAU=2*math.pi
SEED=20261102

# Slot ordering matches original Betty samplebank.h.
# A: kick, snare, rim, percussion; B: closed hat, open hat, tamb, percussion;
# C: tom, rim, triangle, percussion.
SOUNDS=[
    ("sine_kick",      "Sine Kick",       .245, 1.10, 1.30),
    ("noisy_snare",    "Noisy Snare",     .150, 1.38, 2.30),
    ("dry_clap",       "Dry Clap",        .064, 1.38, 2.35),
    ("soft_fm_tick",   "Soft FM Tick",    .046, 1.85, 3.10),
    ("tight_hat",      "Tight Noisy Hat", .046, 1.95, 3.15),
    ("airy_hat",       "Airy Open Hat",   .185, 2.15, 3.50),
    ("sand_shaker",    "Sand Shaker",     .081, 2.12, 3.40),
    ("micro_hat",      "Micro Hat Click", .032, 2.10, 3.50),
    ("fm_tom",         "FM Low Tom",      .158, 1.95, 2.85),
    ("fm_wood",        "FM Woodblock",    .071, 2.05, 3.45),
    ("fm_plick",       "Soft FM Plick",   .076, 2.35, 3.70),
    ("fm_micro_click", "FM Micro Click",  .029, 2.55, 4.10),
]
SOUND_LABELS={entry[0]:entry[1] for entry in SOUNDS}

def clip(x):
    return max(-.91,min(.91,x))

def render(name,duration,seed):
    rng=random.Random(seed)
    n=math.ceil(duration*FS/16)*16
    out=[]
    phase=0.
    modphase=0.
    filtered=0.
    smooth=0.
    prev_noise=0.
    # The 12 sounds have different envelopes and deliberately short tails.
    for i in range(n):
        t=i/FS
        w=rng.uniform(-1,1)
        # 2-pole-like lowpass for softer, darker noise than white-noise drums.
        filtered=.82*filtered+.18*w
        smooth=.78*smooth+.22*filtered
        hp=w-filtered
        band=filtered-smooth
        onset=min(1.,t/.00085)
        x=0.
        if name=="sine_kick":
            hz=48+138*math.exp(-t/.022)
            phase+=TAU*hz/FS
            body=.84*math.sin(phase)*math.exp(-t/.112)
            sub=.13*math.sin(phase*.5)*math.exp(-t/.138)
            tick=.055*hp*math.exp(-t/.0032)
            x=body+sub+tick
        elif name=="noisy_snare":
            phase+=TAU*(174+17*math.exp(-t/.011))/FS
            x=(.43*(.62*hp+.38*band)*math.exp(-t/.049)
               +.16*math.sin(phase)*math.exp(-t/.027))
        elif name=="dry_clap":
            # Microhouse: compact burst (three closely clustered transients).
            # Last transient begins at 10 ms instead of 27 ms, with a short,
            # unobtrusive tail so the clap sounds like one dry event.
            pulse=(.75*math.exp(-(t-.0012)/.0027) if t>=.0012 else 0)
            pulse+=.85*math.exp(-(t-.0050)/.0030) if t>=.0050 else 0
            pulse+=1.0*math.exp(-(t-.0100)/.0045) if t>=.0100 else 0
            tail=.09*math.exp(-t/.017)
            x=.37*(.6*hp+.4*band)*(pulse+tail)
        elif name=="soft_fm_tick":
            f=540;modphase+=TAU*f*2.17/FS
            phase+=TAU*f/FS
            x=.22*math.sin(phase+1.35*math.exp(-t/.009)*math.sin(modphase))*math.exp(-t/.014)
        elif name=="tight_hat":
            x=.29*(.77*hp+.23*band)*math.exp(-t/.012)
        elif name=="airy_hat":
            x=.25*(.63*hp+.37*band)*math.exp(-t/.066)
        elif name=="sand_shaker":
            # A gentle offbeat, continuous textural breath.
            x=.22*(.36*hp+.64*band)*math.exp(-t/.031)
        elif name=="micro_hat":
            x=.16*(.65*hp+.35*band)*math.exp(-t/.0059)
        elif name=="fm_tom":
            carrier=102+58*math.exp(-t/.018)
            phase+=TAU*carrier/FS
            modphase+=TAU*carrier*1.43/FS
            x=.39*math.sin(phase+1.05*math.exp(-t/.034)*math.sin(modphase))*math.exp(-t/.058)
        elif name=="fm_wood":
            carrier=460
            phase+=TAU*carrier/FS
            modphase+=TAU*carrier*2.41/FS
            x=.25*math.sin(phase+2.3*math.exp(-t/.009)*math.sin(modphase))*math.exp(-t/.019)
        elif name=="fm_plick":
            carrier=710
            phase+=TAU*carrier/FS
            modphase+=TAU*carrier*1.62/FS
            x=.23*math.sin(phase+2.0*math.exp(-t/.010)*math.sin(modphase))*math.exp(-t/.021)
        elif name=="fm_micro_click":
            carrier=1150
            phase+=TAU*carrier/FS
            modphase+=TAU*carrier*2.77/FS
            x=.18*math.sin(phase+2.5*math.exp(-t/.0028)*math.sin(modphase))*math.exp(-t/.0065)
        else:
            raise ValueError(name)
        # 3 ms cosine tail to avoid hard end even at shifted Betty playback.
        fade=min(1.,max(0.,(duration-t)/.003))
        out.append(clip(x*onset*fade))
    return out

def brr_encode(pcm):
    """Simple BRR encoder: greedy best error among filters 0 and 1, ranges 0..12."""
    out=[]
    history=0
    for start in range(0,len(pcm),16):
        best=None
        for filter_id in (0,1):
            for shift in range(13):
                step=(1<<shift)
                prev=history
                error=0.
                qvalues=[]
                for target_f in pcm[start:start+16]:
                    pred=int(prev*15/16) if filter_id==1 else 0
                    target=int(target_f*32767)
                    q=max(-8,min(7,round((target-pred)/step)))
                    decoded=max(-32768,min(32767,pred+q*step))
                    error+=(target-decoded)**2
                    prev=decoded
                    qvalues.append(q)
                if best is None or error<best[0]:
                    best=(error,filter_id,shift,qvalues,prev)
        _,filter_id,shift,qvalues,history=best
        end=start+16>=len(pcm)
        out.append((shift<<4)|(filter_id<<2)|(1 if end else 0))
        for j in range(0,16,2):
            out.append(((qvalues[j]&15)<<4)|(qvalues[j+1]&15))
    return out

def write_wav(path,pcm):
    with wave.open(str(path),"wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(FS)
        wav.writeframes(b"".join(struct.pack("<h",max(-32768,min(32767,round(x*32767)))) for x in pcm))

def run():
    preview=Path("preview_wav")
    preview.mkdir(exist_ok=True)
    bank=[
        "#pragma once",
        '#include "userosc.h"',
        "// BTMIC - all 12 sounds synthesized for minimal/micro house.",
        "// Keep same 3x4 ordering and unchanged patterns.h of original Betty.",
        "struct Sample { uint16_t size; float rate; float divAc; float div; const uint8_t *pData; };",
        "static const uint8_t dataSilence[9] = {1,0,0,0,0,0,0,0,0};",
    ]
    total=0
    for idx,(name,label,dur,accent,normal) in enumerate(SOUNDS):
        pcm=render(name,dur,SEED+idx*911)
        encoded=brr_encode(pcm)
        total+=len(encoded)
        bank.append(f"// {idx+1:02d}: {label} ({len(pcm)/FS:.3f} s)")
        bank.append(f"static const uint8_t data_{name}[{len(encoded)}] = {{")
        for j in range(0,len(encoded),18):
            bank.append("  "+", ".join(map(str,encoded[j:j+18]))+",")
        bank.append("};")
        write_wav(preview/f"{idx+1:02d}_{name}.wav",pcm)
        peak=max(abs(x) for x in pcm)
        print(f"{idx+1:02d} {label:20s}  {len(encoded):5d} BRR bytes, peak {peak:.3f}",flush=True)
    bank.append("Sample samples[12] = {")
    for name,label,dur,accent,normal in SOUNDS:
        bank.append(f"  {{.size = 0, .rate = 16000.0f, .divAc = 32768.0f * {accent:.2f}f, .div = 32768.0f * {normal:.2f}f, .pData = &data_{name}[0]}}, // {label}")
    bank.append("};")
    assert total<20000,(total,"Unexpected excessive data size for MKI")
    Path("samplebank.h").write_text("\n".join(bank)+"\n",encoding="utf-8")
    print(f"Created samplebank.h: {total} encoded BRR bytes, 12 original samples",flush=True)

if __name__=="__main__":
    run()
