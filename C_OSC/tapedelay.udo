/***** tapedelay.udo *****/
/********************************************************

	EXPERIMENTAL!

	TapeDelay.udo
	Author: Bernt Isak Wærstad

	Arguments: DelayTime, Feedback, Filter, Distortion, Modulation, Mix
    Defaults:  0.2, 0.4, 0.5

	Delay time: 1ms - 2s
	Feedback: 0% - 120%
	Filter: 200Hz - 12000Hz
	Distortion:
	Modulation: 	
	Mix: 0% - 100%

	Description:
	Analoge style delay

********************************************************/

	; Default argument values

	#define DelayTime #0.2# 
	#define Feedback #0.3#
	#define Filter #0.5#
	#define Distortion #0#	
	#define Modulation #0#
	#define Mix #0.4#

	; Toggle printing on/off
	#define PRINT #0#

;*********************************************************************
; TapeDelay
;*********************************************************************

opcode TapeDelay, a, akkkkkk
	ain, kdlytime, kfeed, kfilter, kdist, kmod, kmix xin

	; ******************************
	; Controller value scalings
	; ******************************

	kdlytime expcurve kdlytime, 10
	kdlytime scale kdlytime, 2000, 1
	kfeed scale kfeed, 1.2, 0
	kfilter scale kfilter, 12000, 200
	kdist scale kdist, 2, 0
	kmod scale kmod, 1, 0
	kmix scale kmix, 1, 0

	if $PRINT == 1 then 
		Scut sprintfk "TapeDelay time: %dms", kdlytime
			puts Scut, kdlytime	

		Sfb sprintfk "TapeDelay Feedback: %f\%", kfeed*100
				puts Sfb, kfeed+1

		Sfilt sprintfk "TapeDelay Filter: %fHz", kfilter
			puts Sfilt, kfilter+1

		Sdist sprintfk "TapeDelay Distortion: %f", kdist
			puts Sdist, kdist+1

		Smod sprintfk "TapeDelay Modulation: %f\%", kmod*100
			puts Smod, kmod+1

		Srev sprintfk "TapeDelay Mix: %f\%", kmix*100
			puts Srev, kmix+1
	endif
	
	kdlytime port kdlytime, 0.7
	adlytime interp kdlytime

	; Delay code
	aFeed init 0

; Modulate delay time too?

	avDly vdelay3 ain + (aFeed * kfeed), adlytime , 3000

	kModfreq = 10
	aLFO oscil kmod, kModfreq*kmod
	

	aFeed lpf18 avDly, kfilter, 0, 0
	
	aLFO *= 0.5
	aLFO += 1
;	aFeed *= aLFO
	

	;----------------------

	aout ntrpol ain, avDly, kmix

	xout aout
endop

;*********************************************************************
; TapeDelay - 1i/2o
;*********************************************************************

opcode TapeDelay, aa, akkkkkk
	ain, kdlytime, kfeed, kfilter, kdist, kmod, kmix xin

	aOutL TapeDelay ain, kdlytime, kfeed, kfilter, kdist, kmod, kmix
	aOutR TapeDelay ain, kdlytime, kfeed, kfilter, kdist, kmod, kmix

	xout aOutL, aOutR
endop

;*********************************************************************
; TapeDelay - 2i/2o
;*********************************************************************

opcode TapeDelay, aa, aakkkkkk
	ainL, ainR, kdlytime, kfeed, kfilter, kdist, kmod, kmix xin

	aOutL TapeDelay ainL, kdlytime, kfeed, kfilter, kdist, kmod, kmix
	aOutR TapeDelay ainR, kdlytime, kfeed, kfilter, kdist, kmod, kmix

	xout aOutL, aOutR
endop


;	avDlyL vdelay3 ainL + (aFeedL * kfeed), adlytime, 3000
;	avDlyR vdelay3 ainR + (aFeedR * kfeed), adlytime*1.02, 3000

