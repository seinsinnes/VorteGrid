(
s.waitForBoot({
    var vorteGrid = NetAddr("127.0.0.1", 9000);
    var clock = TempoClock.new(120 / 60).permanent_(true); // 120 BPM ("The Magick")

    // Master sensitivity knob: 1.0 = subtle organic modulation around defaults, 0.0 = 100% stock undriven flame
    ~modDepth = 1.0;

    // Shared state initialized to exact VorteGrid case 18 defaults
    ~flameState = (
        kickImpulse: 0.0,
        snareImpulse: 0.0,
        targetColor:  [1.0, 0.50, 0.10], // Exact case 18 orange
        currentColor: [1.0, 0.50, 0.10],
        targetSmoke:  [0.70, 0.70, 0.75], // Exact case 18 smoke gray
        currentSmoke: [0.70, 0.70, 0.75]
    );

    // =========================================================================
    // 1. SYNTHDEFS (Witchz - "The Magick" Dark Western / Witch-House Palette)
    // =========================================================================

    SynthDef(\magickKick, {
        |out=0, amp=0.85|
        var pitchEnv = EnvGen.kr(Env([160, 52, 34], [0.035, 0.28], \exp));
        var ampEnv   = EnvGen.kr(Env.perc(0.002, 0.48, 1.0, -5), doneAction: 2);
        var click    = HPF.ar(WhiteNoise.ar(0.3), 900) * EnvGen.kr(Env.perc(0.001, 0.015));
        var body     = SinOsc.ar(pitchEnv) + (SinOsc.ar(pitchEnv * 2) * 0.25);
        var sig      = (body + click).tanh * ampEnv * amp;
        Out.ar(out, sig ! 2);
    }).add;

    SynthDef(\magickSnare, {
        |out=0, amp=0.65|
        var bodyEnv  = EnvGen.kr(Env.perc(0.002, 0.16, 1.0, -4));
        var body     = SinOsc.ar(EnvGen.kr(Env([260, 135], [0.05], \exp))) * bodyEnv;
        var noiseEnv = EnvGen.kr(Env([0, 1, 0.4, 1, 0], [0.002, 0.012, 0.01, 0.28], -4), doneAction: 2);
        var noise    = BPF.ar(PinkNoise.ar(2.2), 1850, 0.65) * noiseEnv;
        var sig      = ((body * 0.6) + noise).softclip * amp;
        Out.ar(out, Pan2.ar(sig, 0.0));
    }).add;

    SynthDef(\magickHat, {
        |out=0, amp=0.3, decay=0.06, pan=0.0|
        var env = EnvGen.kr(Env.perc(0.001, decay, 1.0, -6), doneAction: 2);
        var sig = HPF.ar(WhiteNoise.ar(1.0), 7500) * env * amp;
        Out.ar(out, Pan2.ar(sig, pan));
    }).add;

    SynthDef(\magickReese, {
        |out=0, freq=36.71, amp=0.55, gate=1, cutoff=750, detune=0.012|
        var env      = EnvGen.kr(Env.adsr(0.03, 0.2, 0.85, 0.25), gate, doneAction: 2);
        var fEnv     = EnvGen.kr(Env([cutoff * 1.8, cutoff, cutoff * 0.6], [0.15, 0.6], \exp));
        var saws     = Splay.ar(Saw.ar(freq * [1 - detune, 1, 1 + detune, 2.005]));
        var sub      = SinOsc.ar(freq, 0, 0.8);
        var filtered = RLPF.ar((saws * 0.75) + sub, fEnv.clip(60, 4500), 0.35);
        var sig      = (filtered * 1.8).tanh * env * amp;
        Out.ar(out, sig);
    }).add;

    SynthDef(\magickPluck, {
        |out=0, freq=293.66, amp=0.32, decay=1.4, pan=0.0|
        var env   = EnvGen.kr(Env.perc(0.004, decay, 1.0, -4), doneAction: 2);
        var exc   = PinkNoise.ar(EnvGen.kr(Env.perc(0.001, 0.02))) + SinOsc.ar(freq * 2, 0, 0.2);
        var pluck = Pluck.ar(exc, 1, 0.05, freq.reciprocal, decay, 0.32);
        var bell  = SinOsc.ar(freq * [1, 2.002], 0, [0.25, 0.08]).sum * env;
        var sig   = LPF.ar(pluck + bell, 2600) * env * amp;
        Out.ar(out, Pan2.ar(sig, pan));
    }).add;

    SynthDef(\magickChoir, {
        |out=0, freq=146.83, amp=0.22, gate=1|
        var env = EnvGen.kr(Env.asr(0.4, 1.0, 0.8), gate, doneAction: 2);
        var vib = SinOsc.kr(4.5, 0, 0.004, 1.0);
        var src = Saw.ar(freq * vib * [0.996, 1.0, 1.004, 1.5]);
        var f1  = BPF.ar(src, 620, 0.15) * 3.0;
        var f2  = BPF.ar(src, 1080, 0.18) * 2.2;
        var f3  = BPF.ar(src, 2450, 0.22) * 1.2;
        var sig = Splay.ar((f1 + f2 + f3).tanh) * env * amp;
        Out.ar(out, sig);
    }).add;

    SynthDef(\magickMasterFX, {
        |inBus=0, outBus=0|
        var dry      = In.ar(inBus, 2);
        var mono     = dry.sum * 0.5;
        var subSig   = LPF.ar(mono, 110);
        var subAmp   = Amplitude.kr(subSig, 0.01, 0.15);
        var totalAmp = Amplitude.kr(mono, 0.01, 0.12);
        var fft      = FFT(LocalBuf(1024), mono);
        var centroid = SpecCentroid.kr(fft);
        var verb     = FreeVerb2.ar(dry[0], dry[1], 0.28, 0.82, 0.45);
        var master   = Limiter.ar((verb * 1.15).softclip, 0.95);

        SendReply.kr(Impulse.kr(30), '/audio/telemetry', [subAmp, totalAmp, centroid]);
        ReplaceOut.ar(outBus, master);
    }).add;

    s.sync;

    ~masterFX = Synth.tail(s, \magickMasterFX);

    // Set exact case 18 static defaults on startup
    vorteGrid.sendMsg("/flame/lifetime", 300);
    vorteGrid.sendMsg("/flame/smoke_temp", 5000.0);
    vorteGrid.sendMsg("/flame/smoke_rate", 2.0);
    vorteGrid.sendMsg("/flame/spread", 1.0, 1.0, 0.5);

    // =========================================================================
    // 2. 30 FPS GENTLE OSC FLAME CONTROLLER (Centered on Case 18 Defaults)
    // =========================================================================
    OSCdef(\flameAudioReactive, { |msg|
        var depth    = ~modDepth;
        var subAmp   = (msg[3].clip(0.0, 1.0)) * depth;
        var totalAmp = (msg[4].clip(0.0, 1.0)) * depth;
        var centroid = (msg[5].clip(200.0, 8000.0).explin(200.0, 8000.0, 0.0, 1.0)) * depth;
        var now      = Main.elapsedTime;

        var kImp = ~flameState.kickImpulse * depth;
        var sImp = ~flameState.snareImpulse * depth;

        // Centered around exact case 18 defaults:
        // FreeEnergy = 20000, Fuel = 0.01, PhysDensity = 1.0, VisDensity = 25490, SmokeVis = 100, Scale = 4.0
        var freeEnergy = (20000.0 + (subAmp * 2000.0) + (kImp * 3500.0)).asFloat;
        var fuelFrac   = (0.0100  + (subAmp * 0.0010) + (kImp * 0.0022)).clip(0.008, 0.014).asFloat;
        var physDens   = (1.0000  - (kImp * 0.015)).clip(0.98, 1.0).asFloat;
        var visDens    = (25490.0 + (totalAmp * 1500.0) + (kImp * 2000.0)).asFloat;
        var smokeVis   = (100.0   + (subAmp * 12.0) + (sImp * 10.0)).asFloat;
        var flameScale = (4.00    + (subAmp * 0.12) + (kImp * 0.22)).asFloat;

        // CombustionTemp default = 1201.5 K, CombustionRate default = 2.0
        var combTemp   = (1201.5  - (kImp * 35.0) - (sImp * 45.0)).clip(1120.0, 1220.0).asFloat;
        var combRate   = (2.00    + (totalAmp * 0.15) + (sImp * 0.35)).asFloat;

        // EmitRate defaults = 200.0 vortons/sec, 10000.0 tracers/sec
        var vortonRate = (200.0   + (kImp * 20.0)).asFloat;
        var tracerRate = (10000.0 + (totalAmp * 400.0) + (kImp * 600.0)).asFloat;

        // Whisper-light wind around default (0, 0, 0) so natural buoyancy dominates
        var windStrength = (0.02 + (centroid * 0.04) + (sImp * 0.08)) * depth;
        var windX = (sin(now * 1.5) * windStrength).asFloat;
        var windY = (cos(now * 1.1) * windStrength).asFloat;
        var windZ = ((kImp * 0.10) + (subAmp * 0.04)).asFloat;

        // Smooth interpolation around the natural orange flame palette
        3.do({ |i|
            ~flameState.currentColor[i] = ~flameState.currentColor[i].blend(~flameState.targetColor[i], 0.08);
            ~flameState.currentSmoke[i] = ~flameState.currentSmoke[i].blend(~flameState.targetSmoke[i], 0.08);
        });

        vorteGrid.sendMsg("/flame/color",
            ~flameState.currentColor[0].asFloat,
            ~flameState.currentColor[1].asFloat,
            ~flameState.currentColor[2].asFloat
        );
        vorteGrid.sendMsg("/smoke/color",
            ~flameState.currentSmoke[0].asFloat,
            ~flameState.currentSmoke[1].asFloat,
            ~flameState.currentSmoke[2].asFloat
        );

        vorteGrid.sendMsg("/flame/free_energy", freeEnergy);
        vorteGrid.sendMsg("/flame/fuel_fraction", fuelFrac);
        vorteGrid.sendMsg("/flame/density/physical", physDens);
        vorteGrid.sendMsg("/flame/density/visual", visDens);
        vorteGrid.sendMsg("/smoke/density/visual", smokeVis);
        vorteGrid.sendMsg("/flame/scale", flameScale);
        vorteGrid.sendMsg("/flame/combustion_temp", combTemp);
        vorteGrid.sendMsg("/flame/combustion_rate", combRate);
        vorteGrid.sendMsg("/flame/emit_rate", vortonRate, tracerRate);
        //vorteGrid.sendMsg("/flame/wind", windX, windY, windZ);

        ~flameState.kickImpulse  = ~flameState.kickImpulse * 0.78;
        ~flameState.snareImpulse = ~flameState.snareImpulse * 0.75;
    }, '/audio/telemetry');

    // =========================================================================
    // 3. MUSICAL PATTERNS (Subtle Warm Fire Palette Shifts on Each Bar)
    // =========================================================================

    Pdef(\magickDirector, Pbind(
        \type, \rest,
        \dur, 4,
        \barIdx, Pseq([0, 1, 2, 3], inf),
        \doOsc, Pfunc({ |ev|
            switch(ev[\barIdx],
                0, { // Bar 1 (Dm): Classic Case 18 Orange
                    ~flameState.targetColor = [1.0, 0.50, 0.10];
                    ~flameState.targetSmoke = [0.70, 0.70, 0.75];
                    //vorteGrid.sendMsg("/flame/spread", 1.0, 1.0, 0.5);
                },
                1, { // Bar 2 (Bb): Deep Ember Orange-Red
                    ~flameState.targetColor = [1.0, 0.42, 0.12];
                    ~flameState.targetSmoke = [0.68, 0.66, 0.74];
                    //vorteGrid.sendMsg("/flame/spread", 1.05, 1.05, 0.5);
                },
                2, { // Bar 3 (Gm): Warm Copper-Amber
                    ~flameState.targetColor = [0.98, 0.54, 0.14];
                    ~flameState.targetSmoke = [0.66, 0.70, 0.74];
                    //vorteGrid.sendMsg("/flame/spread", 0.95, 0.95, 0.5);
                },
                3, { // Bar 4 (A): Golden Hearth Flame
                    ~flameState.targetColor = [1.0, 0.58, 0.12];
                    ~flameState.targetSmoke = [0.72, 0.70, 0.72];
                    //vorteGrid.sendMsg("/flame/spread", 1.0, 1.0, 0.5);
                }
            );
        })
    )).play(clock, quant: 4);

    Pdef(\magickKickPat, Pbind(
        \instrument, \magickKick,
        \dur, Pseq([1, 1.5, 0.5, 1,   1, 1, 0.75, 0.25, 1], inf),
        \amp, 0.9,
        \oscTrig, Pfunc({ ~flameState.kickImpulse = 1.0; })
    )).play(clock, quant: 4);

    Pdef(\magickSnarePat, Pbind(
        \instrument, \magickSnare,
        \dur, Pseq([Rest(1), 1, Rest(1), 1], inf),
        \amp, 0.7,
        \oscTrig, Pfunc({ ~flameState.snareImpulse = 1.0; })
    )).play(clock, quant: 4);

    Pdef(\magickHatPat, Pbind(
        \instrument, \magickHat,
        \dur, Pseq([
            0.5, 0.5, 0.25, 0.25, 0.25, 0.25,
            0.5, 0.25, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.5
        ], inf),
        \decay, Pseq([0.05, 0.04, 0.03, 0.03, 0.08, 0.04, 0.12], inf),
        \amp, Pseq([0.28, 0.18, 0.22, 0.16, 0.32, 0.15, 0.25], inf),
        \pan, Pwhite(-0.45, 0.45, inf)
    )).play(clock, quant: 4);

    Pdef(\magickBassPat, Pbind(
        \instrument, \magickReese,
        \midinote, Pseq([38, 34, 31, 33], inf),
        \dur, 4,
        \legato, 0.95,
        \cutoff, Pseq([700, 950, 620, 1250], inf),
        \amp, 0.55
    )).play(clock, quant: 4);

    Pdef(\magickArpPat, Pbind(
        \instrument, \magickPluck,
        \midinote, Pseq([
            62, 65, 69, 62,  70, 69, 65, 62,
            58, 62, 65, 70,  69, 65, 62, 58,
            55, 58, 62, 67,  65, 62, 58, 55,
            57, 61, 64, 69,  70, 69, 64, 61
        ], inf),
        \dur, 0.5,
        \decay, Pexprand(0.9, 1.8, inf),
        \amp, Pseq([0.34, 0.22, 0.28, 0.20], inf),
        \pan, Pseq([-0.35, 0.35, -0.2, 0.2], inf)
    )).play(clock, quant: 4);

    Pdef(\magickChoirPat, Pbind(
        \instrument, \magickChoir,
        \midinote, Pseq([
            [50, 57, 62],
            [46, 53, 58],
            [43, 50, 55],
            [45, 52, 61]
        ], inf),
        \dur, 4,
        \legato, 0.98,
        \amp, 0.18
    )).play(clock, quant: 4);

    "=== THE MAGICK: Natural Case 18 Flame + Subtle Beat Modulation Active ===".postln;
});
)

//~modDepth = 2.0;