/***** _main.csd *****/
<CsoundSynthesizer>
<CsOptions>
-m0d
</CsOptions>
<CsInstruments>
ksmps = 8
nchnls = 2
0dbfs = 1
;------------------------------------------------------
;  	Simple AM synth with analoge in and out
;	See comment below for wiring
;------------------------------------------------------
	;gihandle BelaOSCinit 9999

	instr 1

	ihandle = 0
	idest = "/something"

	ipin = 0
	ksig digiInBela ipin

	ksig digiInBela2 ipin

	;kp bOSClisten ihandle, idest	

	

	endin

</CsInstruments>
<CsScore>
i1 0 86400
</CsScore>
</CsoundSynthesizer>