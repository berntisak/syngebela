/*
  BelaCsound.cpp:

  Copyright (C) 2017 V Lazzarini

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/

#include <getopt.h>
#include <Bela.h>
#include <Midi.h>
#include <Scope.h>
#include <csound/csound.hpp>
#include <csound/plugin.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <atomic>

#include <OSCServer.h>

OSCServer oscServer;

static int OpenMidiInDevice(CSOUND *csound, void **userData, const char *dev);
static int CloseMidiInDevice(CSOUND *csound, void *userData);
static int ReadMidiData(CSOUND *csound, void *userData, unsigned char *mbuf,
			int nbytes);
static int OpenMidiOutDevice(CSOUND *csound, void **userData, const char *dev);
static int CloseMidiOutDevice(CSOUND *csound, void *userData);
static int WriteMidiData(CSOUND *csound, void *userData, const unsigned char *mbuf,
			 int nbytes);

/** DigiIn opcode 
    ksig digiInBela ipin
    asig digiInBela ipin
*/
struct DigiIn : csnd::Plugin<1, 1> {
  int pin;
  int fcount;
  int frms;
  int init_done;
  BelaContext *context;
  
  int init() {
    pin = (int) inargs[0];
    if(pin < 0 ) pin = 0;
    if(pin > 15) pin = 15;
    context = (BelaContext *) csound->host_data();
    fcount = 0;
    init_done = 0;
    frms = context->digitalFrames; 
    return OK;
  }

  int kperf() {
   if(!init_done) {
      pinMode(context,0,pin,0);
      init_done = 1;
    }
    outargs[0] = (MYFLT) digitalRead(context,fcount,pin);
    fcount += nsmps;
    fcount %= frms;
    return OK;
  }

  int aperf() {
    csnd::AudioSig out(this, outargs(0));
    int cnt = fcount;
    if(!init_done) {
      pinMode(context,0,pin,0);
      init_done = 1;
    }
    for (auto &s : out) {
      s = (MYFLT) digitalRead(context,cnt,pin);
      if(cnt == frms - 1) cnt = 0;
      else cnt++;
    }
    fcount = cnt;
    return OK;
  }
};

/** DigiOut opcode 
    digiOutBela ksig,ipin
    digiOutBela asig,ipin
*/
struct DigiOut : csnd::Plugin<0, 2> {
  int pin;
  int fcount;
  int frms;
  int init_done;
  BelaContext *context;
  
  int init() {
    pin = (int) inargs[1];
    if(pin < 0 ) pin = 0;
    if(pin > 15) pin = 15;
    context = (BelaContext *) csound->host_data();
    init_done = 0;
    fcount = 0;
    frms = context->digitalFrames; 
    return OK;
  }

  int kperf() {
    if(!init_done) {
      pinMode(context,0,pin,1);
      init_done = 1;
    }
    digitalWrite(context,fcount,pin,(inargs[0] > 0.0 ? 1 : 0));
    fcount += nsmps;
    fcount %= frms;
    return OK;
  }

  int aperf() {
    csnd::AudioSig in(this, inargs(0));
    int cnt = fcount;
    if(!init_done) {
      pinMode(context,0,pin,1);
      init_done = 1;
    }
    for (auto s : in) {
      digitalWriteOnce(context,cnt,pin, (s > 0.0 ? 1 : 0));
      if(cnt == frms - 1) cnt = 0;
      else cnt++;
    }
    fcount = cnt;
    return OK;
  }
};

/** DigiIO opcode 
    allows change of direction & pin
    ksig/asig are used for input or output
    digiIOBela ksig,kpin,kdir
    digiIOBela asig,apin,adir 
*/
struct DigiIO : csnd::Plugin<0, 3> {
  int fcount;
  int frms;
  BelaContext *context;
  
  int init() {
    context = (BelaContext *) csound->host_data();
    fcount = 0;
    frms = context->digitalFrames; 
    return OK;
  }

  int kperf() {
    int pin = (int) inargs[1];
    if(pin < 0 ) pin = 0;
    if(pin > 15) pin = 15;
    int mode = inargs[2] > 0.0 ? 1 : 0;
    pinMode(context,fcount,pin,mode);
    if(mode)
     digitalWrite(context,fcount,pin,(inargs[0] > 0.0 ? 1 : 0));
    else
     inargs[0] = (MYFLT) digitalRead(context,fcount,pin);
    fcount += nsmps;
    fcount %= frms;
    return OK;
  }

  int aperf() {
    csnd::AudioSig sig(this, inargs(0));
    csnd::AudioSig pins(this, inargs(1));
    csnd::AudioSig modes(this, inargs(2));
    int cnt = fcount;
    int n = 0, pin, mode;
    for (auto &s : sig) {
      pin = pins[n];
      if(pin < 0 ) pin = 0;
      if(pin > 15) pin = 15;
      mode = modes[n] > 0.0 ? 1 : 0;
      pinModeOnce(context,cnt,pin,mode);
      if(mode)
       digitalWriteOnce(context,cnt,pin,(s > 0.0 ? 1 : 0));
      else
       s = (MYFLT) digitalRead(context,cnt,pin);
      if(cnt == frms - 1) cnt = 0;
      else cnt++;
      n++;
    }
    fcount = cnt;
    return OK;
  }
};

struct CsChan {
  std::vector<MYFLT> samples;
  std::stringstream name;
};

struct CsData {
  Csound *csound;
  std::string csdfile;
  int blocksize;
  std::atomic_int res;
  int count;
  std::vector<CsChan> channel;
  std::vector<CsChan> ochannel;
  CsChan schannel;
  Scope scope;
};

struct BelaOSCinit : csnd::Plugin<1,1>
{
    
    int port;
    
    int init() {
    	int port = (int) inargs[0];
        oscServer.setup(port);
        rt_printf("Listening on port %i", port);
        return OK; 
    }
    
};

constexpr int NumOscMsgs = 15;

struct BelaOSClisten : csnd::Plugin<1, 1 + NumOscMsgs*2>
{
    
    int init()
    {
        return OK;
    }
    
    int kperf()
    {
        while (oscServer.messageWaiting()){
            oscpkt::Message p = oscServer.popMessage();

            for (int i = 0; i < NumOscMsgs; ++i) {
				auto address = inargs.str_data(1 + i).data;
				auto oscargs = p.match(address);
				float a;

				if (!oscargs) continue; // no address match
				//rt_printf("%s\n", address);
				if (oscargs.popFloat(a).isOkNoMoreArgs())
					*(inargs.data(1 + NumOscMsgs + i)) = a;
				else
					rt_printf("%s: got wrong number of params\n", address);
            }
        }
        return OK; 
    }
};

bool csound_setup(BelaContext *context, void *p)
{
  CsData *csData = (CsData *) p;
  Csound *csound;
  const char *args[] = { "csound", csData->csdfile.c_str(), "-iadc",
			 "-odac", "-+rtaudio=null",
			 "--realtime", "--daemon" };
  int numArgs = (int) (sizeof(args)/sizeof(char *));

  if(context->audioInChannels != context->audioOutChannels) {
    printf("Error: number of audio inputs != number of audio outputs.\n");
    return false;
  }

  /* allocate analog channel memory */
  csData->channel.resize(context->analogInChannels);
  csData->ochannel.resize(context->analogOutChannels);
  
  /* set up Csound */
  csound = new Csound();
  csData->csound = csound;
  csound->SetHostData((void *) context);
  csound->SetHostImplementedAudioIO(1,0);
  csound->SetHostImplementedMIDIIO(1);
  csound->SetExternalMidiInOpenCallback(OpenMidiInDevice);
  csound->SetExternalMidiReadCallback(ReadMidiData);
  csound->SetExternalMidiInCloseCallback(CloseMidiInDevice);
  csound->SetExternalMidiOutOpenCallback(OpenMidiOutDevice);
  csound->SetExternalMidiWriteCallback(WriteMidiData);
  csound->SetExternalMidiOutCloseCallback(CloseMidiOutDevice);
  /* set up digi opcodes */
  if(csnd::plugin<DigiIn>((csnd::Csound *) csound->GetCsound(), "digiInBela",
			  "k","i", csnd::thread::ik) != 0)
    printf("Warning: could not add digiInBela k-rate opcode\n");
  if(csnd::plugin<DigiIn>((csnd::Csound *) csound->GetCsound(), "digiInBela",
			  "a", "i", csnd::thread::ia) != 0)
    printf("Warning: could not add digiInBela a-rate opcode\n");
  if(csnd::plugin<DigiOut>((csnd::Csound *) csound->GetCsound(), "digiOutBela" ,
			   "", "ki", csnd::thread::ik) != 0)
    printf("Warning: could not add digiOutBela k-rate opcode\n");
  if(csnd::plugin<DigiOut>((csnd::Csound *) csound->GetCsound(), "digiOutBela" ,
			   "", "ai", csnd::thread::ia) != 0)
    printf("Warning: could not add digiOutBela a-rate opcode\n");
  if(csnd::plugin<DigiIO>((csnd::Csound *) csound->GetCsound(), "digiIOBela" ,
			   "", "kkk", csnd::thread::ik) != 0)
    printf("Warning: could not add digiIOBela k-rate opcode\n");
  if(csnd::plugin<DigiIO>((csnd::Csound *) csound->GetCsound(), "digiIOBela" ,
			   "", "aaa", csnd::thread::ia) != 0)
    printf("Warning: could not add digiIOBela a-rate opcode\n");
  
  if(csnd::plugin<BelaOSCinit>((csnd::Csound *) csound->GetCsound(), "BelaOSCinit" ,
                "i", "i", csnd::thread::i) != 0)
    printf("Warning: could not add BelaOSCinit i-rate opcode\n");
  if(csnd::plugin<BelaOSClisten>((csnd::Csound *) csound->GetCsound(), "BelaOSClisten" ,
                "k", "iSSSSSSSSSSSSSSSkkkkkkkkkkkkkkk", csnd::thread::k) != 0)
    printf("Warning: could not add BelaOSCinit k-rate opcode\n");

  
  
  /* compile CSD */  
  if((csData->res = csound->Compile(numArgs, args)) != 0) {
    printf("Error: Csound could not compile CSD file.\n");
    return false;
  }
  csData->blocksize = csound->GetKsmps()*csound->GetNchnls();
  csData->count = 0;

  /* set up the channels */
  for(unsigned int i = 0; i < csData->channel.size(); i++) {
    csData->channel[i].samples.resize(csound->GetKsmps());
    csData->channel[i].name << "analogIn" << i;
  }

  for(unsigned int i = 0; i < csData->ochannel.size(); i++) {
    csData->ochannel[i].samples.resize(csound->GetKsmps());
    csData->ochannel[i].name << "analogOut" << i;
  }

  csData->schannel.samples.resize(csound->GetKsmps());
  csData->schannel.name << "scope";
  csData->scope.setup(1, context->audioSampleRate);
  
  return true;
}

void csound_render(BelaContext *context, void *p)
{
  CsData *csData = (CsData *) p;
  if(csData->res == 0) {
    unsigned int i,k,count, frmcount,blocksize;
    int res = csData->res;
    unsigned int n;
    Csound *csound = csData->csound;
    MYFLT scal = csound->Get0dBFS();
    MYFLT* audioIn = csound->GetSpin();
    MYFLT* audioOut = csound->GetSpout();
    int nchnls = csound->GetNchnls();
    unsigned int chns = (unsigned int) nchnls < context->audioOutChannels ?
      nchnls : context->audioOutChannels;
    std::vector<CsChan> &channel = csData->channel;
    std::vector<CsChan> &ochannel = csData->ochannel;
    CsChan &schannel = csData->schannel;
    Scope &scope = csData->scope;
    float frm = 0.f, incr =
      ((float) context->analogFrames)/context->audioFrames;
    count = csData->count;
    blocksize = csData->blocksize;
      
    /* processing loop */
    for(n = 0; n < context->audioFrames; n++, frm+=incr, count+=nchnls){
      if(count == blocksize) {
	
	/* set the channels */
	for(i = 0; i < channel.size(); i++) 
          csound->SetChannel(channel[i].name.str().c_str(),
			     channel[i].samples.data());
	 
	/* run csound */
	if((res = csound->PerformKsmps()) == 0) count = 0;
	else break;

        /* get the channels */
        for(i = 0; i < ochannel.size(); i++) 
	  csound->GetAudioChannel(ochannel[i].name.str().c_str(),
				  ochannel[i].samples.data());
	
        /* get the scope data */
        csound->GetAudioChannel(schannel.name.str().c_str(),
				  schannel.samples.data());
	
      }
      /* read/write audio data */
      for(i = 0; i < chns; i++){
	 audioIn[count+i] = audioRead(context,n,i)*scal;
	 audioWrite(context,n,i,audioOut[count+i]/scal);
      }
      
      /* read analogue data 
         analogue frame pos frm gets incremented according to the
         ratio analogFrames/audioFrames.
      */
      frmcount = count/nchnls;
      k = (int) frm;
      for(i = 0; i < channel.size(); i++) 
        channel[i].samples[frmcount] = analogRead(context,k,i);

      /* write analogue data */
      for(i = 0; i < ochannel.size(); i++) 
	analogWriteOnce(context,k,i,ochannel[i].samples[frmcount]); 
      
      scope.log(schannel.samples[frmcount]);
    }
    csData->res = res;
    csData->count = count;
  }
}

void csound_cleanup(BelaContext *context, void *p)
{
  CsData *csData = (CsData *) p;
  delete csData->csound;
}

static Midi gMidi;
/** MIDI functions 
 */
int OpenMidiInDevice(CSOUND *csound, void **userData, const char *dev) {
  Midi *midi = &gMidi;
  if(midi->readFrom(dev) == 1) {
    midi->enableParser(false);
    *userData = (void *) midi;
    return 0;
  }
  csoundMessage(csound, "Could not open Midi in device %s", dev);
  return -1;
}

int CloseMidiInDevice(CSOUND *csound, void *userData) {
  return 0;
}
 
int ReadMidiData(CSOUND *csound, void *userData,
		 unsigned char *mbuf, int nbytes) {
  int n = 0, byte;
  if(userData) {
    Midi *midi = (Midi *) userData;
    while((byte = midi->getInput()) >= 0) {
      *mbuf++ = (unsigned char) byte;
      if(++n == nbytes) break;
    }
    return n;
  }
  return 0;
}

int OpenMidiOutDevice(CSOUND *csound, void **userData, const char *dev) {
  Midi *midi = &gMidi;
  if(midi->writeTo(dev) == 1) {
    midi->enableParser(false);
    *userData = (void *) midi;
    return 0;
  }
  csoundMessage(csound, "Could not open Midi out device %s", dev);
  return -1;
}

int CloseMidiOutDevice(CSOUND *csound, void *userData) {
  return 0;
}

int WriteMidiData(CSOUND *csound, void *userData,
		  const unsigned char *mbuf, int nbytes) {
  if(userData) {
    Midi *midi = (Midi *) userData;
    if(midi->writeOutput((midi_byte_t *)mbuf, nbytes) > 0) return nbytes;
    return 0;
  }
  return 0;
}

void usage(const char *prg) {
  std::cerr << prg << " [options]\n";
  Bela_usage();
  std::cerr << "  --csd=name [-f name]: CSD file name\n";
  std::cerr << "  --help [-h]: help message\n";	   
}

/**
   Main program: takes Bela options and a --csd=<csdfile> 
   option for Csound
*/
int main(int argc, const char *argv[]) {
  CsData csData;
  int c;
  bool res = false;
  BelaInitSettings settings;
  const option opt[] = {{"csd", required_argument, NULL, 'f'},
			{"help", 0, NULL, 'h'},
			{NULL, 0, NULL, 0}};
  
  Bela_defaultSettings(&settings);
  settings.setup = csound_setup;
  settings.render = csound_render;
  settings.cleanup = csound_cleanup;
  settings.highPerformanceMode = 1;
  settings.interleave = 1;
  settings.analogOutputsPersist = 0;

  while((c = Bela_getopt_long(argc, (char **) argv, "hf", opt, &settings)) >= 0) {
    if (c == 'h') {
      usage(argv[0]);
      return 1;
    } else if (c == 'f') {
      csData.csdfile = optarg;
      res = true;
    } else {
      usage(argv[0]);
      return 1;
    }
  }
  
  if(res) {
    res = Bela_initAudio(&settings, &csData);
    if(!res){
      if(Bela_startAudio() == 0) {
         while(csData.res == 0)
	   usleep(100000);
      } else
	std::cerr << "error starting audio \n";
      Bela_stopAudio();
    } else
      std::cerr << "error initialising Bela \n";
    Bela_cleanupAudio();
    return 0;
  }
  std::cerr << "no csd provided, use --csd=name \n";
  return 1;
}
