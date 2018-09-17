/***** osc.csd *****/
<CsoundSynthesizer>
<CsOptions>
-m0d
</CsOptions>
<CsInstruments>
ksmps = 16
nchnls = 2
0dbfs = 1

;giSmpl1 ftgen 0, 0, 1, "spor1.wav", 0, 0, 0
;giSmpl2 ftgen 0, 0, 1, "spor2.wav", 0, 0, 0
;giSmpl3 ftgen 0, 0, 1, "spor3.wav", 0, 0, 0

;giSoundFile   ftgen   0, 0, 262144, 1, "loop.wav", 0, 0, 0

;------------------------------------------------------
;	Audio in/out and analoge in to control gain
;------------------------------------------------------

	#include "tapedelay.udo"
	#include "lowpass.udo"
	#include "highpass.udo"
	#include "reverb.udo"
	#include "reverse.udo"
	
	gidans1 BelaOSCinit 9999
	
	instr 1
		aL, aR ins
		aMono = aL + aR
		aMono butterhp aMono, 120
		aMono pareq aMono, 800, ampdb(-3), 1.5, 0
		aMono pareq aMono, 400, ampdb(-3), 1.5, 0
		aMono pareq aMono, 1200, ampdb(-2), 2, 0
		aMono pareq aMono, 3000, ampdb(6), 0.5, 2
		
		kRevMix init 0
		kRevSize init 0.85
		
		kDlyMix init 0
		kDlyTime init 0.5
		kDlyFeed init 0.6
		
		kReverseMix init 0
		kHP_Cutoff init 0
		
		kStopSamples init 0
		kSample1Play init 0
		kSample2Play init 0
		kSample3Play init 0
		kSample4Play init 0
		kSample5Play init 0
		gkSampleVolume init 1
		
		kMicVol init 1
		aMono *= kMicVol
		
		#include "/root/bela_id.inc"

		knotused BelaOSClisten gidans1, "/Delay/mix", "/Delay/time", "/Delay/feed","/Reverb/mix", "/Reverb/size", \
		"/Sample/stop", "/Sample/1", "/Sample/2", "/Sample/3", "/Sample/4", "/Sample/5", "/Sample/volume", \
		"/Reverse/mix", "/Filter/highpass", Sbela_id, kDlyMix, kDlyTime, kDlyFeed, kRevMix, kRevSize, \
		kStopSamples, kSample1Play, kSample2Play, kSample3Play, kSample4Play, kSample5Play, gkSampleVolume, \
		kReverseMix, kHP_Cutoff, kMicVol

		if kStopSamples == 1 then
			turnoff2 2, 0, 0
		endif
		
		
		kSample1PlayTrig = kSample1Play == 1 && changed(kSample1Play) == 1 ? 1 : 0 
		kSample1Length = ftlen(1)/sr

		kSample2PlayTrig = kSample2Play == 1 && changed(kSample2Play) == 1 ? 1 : 0 
		kSample2Length = ftlen(2)/sr
		
		printk2 kSample2Play
		printk2 kSample3Play
		
		kSample3PlayTrig = kSample3Play == 1 && changed(kSample3Play) == 1 ? 1 : 0 
		kSample3Length = ftlen(3)/sr
		
		; ktrigger, kmintim, kmaxnum, kinsnum, kwhen, kdur [, ip4] [, ip5] [...]
		schedkwhen kSample1PlayTrig, 0, 0, 2, 0, kSample1Length, 1
		schedkwhen kSample2PlayTrig, 0, 0, 2, 0, kSample2Length, 2
		schedkwhen kSample3PlayTrig, 0, 0, 2, 0, kSample3Length, 3
		
	; Filtering
/*
	Arguments: Cutoff_frequency, Resonance, Distortion
    Defaults:  0.8, 0.3, 0

	Cutoff frequency: 30Hz - 12000Hz
	Resonance: 0 - 0.9
	Distortion: 0 - 0.9
	Mode:
		0: lpf18 
		1: moogladder
		2: k35
		3: zdf

*/
		aMono Highpass aMono, kHP_Cutoff*0.3, 0.1, 0, 3

/* 

	Arguments: Reverse_time, Speed, Dry/wet mix
    Defaults:  0.1, 0, 0.5

	Reverse time: 0.1s - 3s
	Speed: 1x - 5x
	Dry/wet mix: 0% - 100%
	
*/
		aMono Reverse aMono, 0.75, 0, kReverseMix

	; Arguments: DelayTime, Feedback, Filter, Distortion, Modulation, Mix

		aMono_Basscut butterhp aMono, 250

		aDlyL,aDlyR TapeDelay aMono_Basscut, kDlyTime, kDlyFeed, 0.8, 0.5, 0.1, 1
		aDlyL *= kDlyMix
		aDlyR *= kDlyMix
		
		kRevSize scale kRevSize, 0.98, 0.8
		
/*
	Arguments: DecayTime, HighFreq_Cutoff, DryWet_Mix, Mode
    Defaults:  0.85, 0.5, 0.5, 0

	Decay Time: 0.1 - 1
	Dampening/cutoff freq: 200Hz - 12000Hz
	Dry/wet mix: 0% - 100%
	Mode: 

		0: reverbsc
		1: freeverb

*/
		;arevL, arevR reverbsc aMono_Basscut+aDlyL, aMono_Basscut+aDlyR, kRevSize, 4000
		arevL, arevR Reverb aMono_Basscut+aDlyL, aMono_Basscut+aDlyR, kRevSize, 0.5, 1, 0
		
		arevL *= kRevMix
		arevR *= kRevMix
		
		outs aDlyL+arevL+aMono,aDlyR+arevR+aMono
	endin

	; SAMPLE PLAYBACK
	
	instr 2
	
	asigL, asigR loscil .8, 1, p4, 1

    outs asigL*gkSampleVolume, asigR*gkSampleVolume
    
	endin

</CsInstruments>
<CsScore>
i1 1 86400
f1 0 0 1 "spor1.wav" 0 0 0
f2 0 0 1 "spor2.wav" 0 0 0
f3 0 0 1 "spor3.wav" 0 0 0
f4 0 0 1 "spor4.wav" 0 0 0
f5 0 0 1 "spor5.wav" 0 0 0

</CsScore>
</CsoundSynthesizer>
