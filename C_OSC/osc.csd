/***** osc.csd *****/
<CsoundSynthesizer>
<CsOptions>
-m0d
</CsOptions>
<CsInstruments>
ksmps = 32
nchnls = 2
0dbfs = 1

;------------------------------------------------------
;	Audio in/out and analoge in to control gain
;------------------------------------------------------

	#include "tapedelay.udo"
	#include "pitchshifter.udo"
	
	gidans1 BelaOSCinit 9999

	instr 1
		aL, aR ins
		
		klfo oscil 0.4, 0.2
		kTrig metro  0.5
		klfo += 0.5
		
		
		;ain, kPitch, kFeedback, kDelay, kDryWet, kMode  xin

		
		; ktrigger, kmintim, kmaxnum, kinsnum, kwhen, kdur [, ip4] [, ip5] [...]
;		schedkwhen kTrig, 0, 0, 2, 0, 1, 3
		
		kRevMix init 0.5
		kDlyMix init 0.3
		kDlyTime init 0.5
		kDlyFeed init 0.3
		klol1 init 0.
		klol2 init 0.
		klol3 init 0.
		klol4 init 0.
		klol5 init 0.
		klol6 init 0.
		
;		kk1 BelaOSClisten gidans1, "/dans1/rev/mix", "f", kRevMix
;		kRevMix BelaOSClisten gidans1, "/Fader1/x"

;		kk2 BelaOSClisten gidans1, "/dans1/dly/mix", "f", kDlyMix
;		kk3 BelaOSClisten gidans1, "/dans1/dly/time", "f", kDlyTime
	
;		kk2 BelaOSClisten gidans1, "/Fader2/x", "f", kDlyMix	
;		kk3 BelaOSClisten gidans1, "/Fader3/x", "f", kDlyTime
;		kk4 BelaOSClisten gidans1, "/Fader4/x", "f", kDlyFeed
;		kDlyFeed BelaOSClisten gidans1, "/Fader4/x"
		knotused BelaOSClisten gidans1, "/1/fader1", "/1/fader2", "/1/fader3", "/1/fader4", "/1/toggle1", "/1/toggle2", klol1, klol2, klol3, klol4, klol5, klol6

	; Arguments: DelayTime, Feedback, Filter, Distortion, Modulation, Mix

		aL,aR TapeDelay aL,aR, kDlyTime, kDlyFeed, 0.8, 0.5, 0.1, kDlyMix
		
		arevL, arevR reverbsc aL, aR, 0.85, 4000

		
		arevL *= kRevMix
		arevR *= kRevMix
		
		outs aL+arevL,aR+arevR
	endin

	; SAMPLE PLAYBACK
	
	instr 2
	
	asig flooper2 1, 1, 0, 2, 0.025, p4
	

    out asig
    
	endin

</CsInstruments>
<CsScore>
i1 0 86400
f2 0 0 1 "fox.wav"0 0 0
f3 0 0 1 "beats.wav" 0 0 0

</CsScore>
</CsoundSynthesizer>
