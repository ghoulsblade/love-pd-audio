// myhello.cpp  ->  myhello.dll (msvc6 : dll example project, select c++:codegen:"multithreaded dll" and add luabin include&lib paths)
//~ #include <openal.h>
// win:vc6 : add openal.lib ?  to linker (and paths accordingly)
// http://connect.creativelabs.com/openal/Documentation/OpenAL%201.1%20Specification.htm
// http://connect.creativelabs.com/openal/Documentation/OpenAL_Programmers_Guide.pdf

#include <stdio.h>
extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
}
#define PROJECT_TABLENAME "lovepdaudio"

#include "z_libpd.h"

#ifdef LUA_API
#undef LUA_API
#endif
#ifdef WIN32
#define LUA_API __declspec(dllexport)
#else
#define LUA_API
#endif

#ifdef WIN32
#else
#include <unistd.h> // Sleep
#endif

// OpenAL
#ifdef LOVE_MACOSX
#include <OpenAL/alc.h>
#include <OpenAL/al.h>
#else
#include <AL/alc.h>
#include <AL/al.h>
#endif

#include <float.h>

extern "C" {
	int LUA_API luaopen_lovepdaudio (lua_State *L);
}

// ***** ***** ***** ***** ***** cLuaAudio

class cLuaAudio { public:
	// The OpenAL device.
	ALCdevice * device;

	// The OpenAL capture device (microphone).
	ALCdevice * capture;

	// The OpenAL context.
	ALCcontext * context;
	
	static const int NUM_SOURCES = 4;
	// OpenAL sources
	ALuint sources[NUM_SOURCES];
	
	ALuint makeSource () { return sources[0]; }

	/// constructor
	cLuaAudio () : device(0),capture(0),context(0) {

		// Passing zero for default device.
		device = alcOpenDevice(0);

		if (device == 0) { fail("Could not open device."); return; }

		context = alcCreateContext(device, 0);

		if (context == 0) { fail("Could not create context."); return; }

		alcMakeContextCurrent(context);

		if (alcGetError(device) != ALC_NO_ERROR) { fail("Could not make context current."); return; }
		
		
		// Generate sources.
		alGenSources(NUM_SOURCES, sources);

		if (alGetError() != AL_NO_ERROR) { fail("Could not generate sources."); return; }
		
		
		printf("cLuaAudio init ok\n");
	}
	
	~cLuaAudio () {
		// Free all sources.
		alDeleteSources(NUM_SOURCES, sources);
		
		
		alcMakeContextCurrent(0);
		alcDestroyContext(context);
		//if (capture) alcCaptureCloseDevice(capture);
		alcCloseDevice(device);
	}
	
	void fail (const char* txt) { printf("cLuaAudio:fail %s\n",txt); }
};

// ***** ***** ***** ***** ***** cLuaAudioDecoder

/// base class
class cLuaAudioDecoder { public:
	static const int DEFAULT_SAMPLE_RATE = 44100;

	cLuaAudioDecoder () {}
	
	virtual bool isFinished () { return false; }
	virtual void* getBuffer () const { return 0; } ///< override me
	virtual int decode () { return 0; } ///< override me
	virtual int getChannels () { return 1; } // mono
	virtual int getBits () { return 8; } // 8bit
	virtual int getSampleRate () { return DEFAULT_SAMPLE_RATE; } // 44k
};

/// dummy to keep code similar to love
class cLuaAudioDecoder_Dummy : public cLuaAudioDecoder { public:
	unsigned char mybuf[10*1024];

	cLuaAudioDecoder_Dummy () {
		for (int i=0;i<sizeof(mybuf)/(getBits()/8);++i) mybuf[i] = (i/1) % 255;
	}
	
	virtual bool isFinished () { return false; }
	virtual void* getBuffer () const { return (void*)mybuf; }
	virtual int decode () { return sizeof(mybuf)/(getBits()/8); } // TODO
	virtual int getChannels () { return 1; } // mono
	virtual int getBits () { return 8; } // 8bit
	virtual int getSampleRate () { return DEFAULT_SAMPLE_RATE; } // 44k
};

// ***** ***** ***** ***** ***** cLuaAudioStream

/// based on love2d::Source
class cLuaAudioStream { public:
	typedef enum { TYPE_STREAM, TYPE_STATIC } eType;
	eType type;
	cLuaAudioDecoder* decoder;
	
	ALuint source;
	bool valid;
	static const unsigned int MAX_BUFFERS = 32;
	ALuint buffers[MAX_BUFFERS];
	
	
	float pitch;
	float volume;
	float minVolume;
	float maxVolume;
	float referenceDistance;
	float rolloffFactor;
	float maxDistance;

	float offsetSamples;
	float offsetSeconds;
	
	/// constructor
	cLuaAudioStream(cLuaAudioDecoder* decoder=0);
	/// destructor
	~cLuaAudioStream();
	
	void	setSource		(ALuint v);
	
	void	playAtomic		();
	bool	update			();
	int		streamAtomic	(ALuint buffer, cLuaAudioDecoder * d);
	ALenum	getFormat		(int channels, int bits) const;
};

// ***** ***** ***** ***** ***** cLuaAudioStream impl

cLuaAudioStream::cLuaAudioStream	(cLuaAudioDecoder* decoder) : type(TYPE_STREAM), valid(false), source(0),
		pitch(1.0f), volume(1.0f), minVolume(0.0f),
		maxVolume(1.0f), referenceDistance(1.0f), rolloffFactor(1.0f), maxDistance(FLT_MAX),
		offsetSamples(0), offsetSeconds(0), decoder(decoder ? decoder : (new cLuaAudioDecoder_Dummy())){
	alGenBuffers(MAX_BUFFERS, buffers);
}

cLuaAudioStream::~cLuaAudioStream	() {
	alDeleteBuffers(MAX_BUFFERS, buffers);
}

void cLuaAudioStream::setSource (ALuint v) {
	source = v;
	valid = true;
}

/// love2d::source::playAtomic
void cLuaAudioStream::playAtomic() {
	if (type == TYPE_STATIC)
	{
		alSourcei(source, AL_BUFFER, buffers[0]);
	}
	else if (type == TYPE_STREAM)
	{
		int usedBuffers = 0;

		for (unsigned int i = 0; i < MAX_BUFFERS; i++)
		{
			streamAtomic(buffers[i], decoder);
			++usedBuffers;
			if (decoder->isFinished())
				break;
		}

		if (usedBuffers > 0)
			alSourceQueueBuffers(source, usedBuffers, buffers);
	}

	// Set these properties. These may have changed while we've
	// been without an AL source.
	alSourcef(source, AL_PITCH, pitch);
	alSourcef(source, AL_GAIN, volume);
	alSourcef(source, AL_MIN_GAIN, minVolume);
	alSourcef(source, AL_MAX_GAIN, maxVolume);
	alSourcef(source, AL_REFERENCE_DISTANCE, referenceDistance);
	alSourcef(source, AL_ROLLOFF_FACTOR, rolloffFactor);
	alSourcef(source, AL_MAX_DISTANCE, maxDistance);

	alSourcePlay(source);

	valid = true; //if it fails it will be set to false again
	//but this prevents a horrible, horrible bug
}

/// love2d::source::update if (type == TYPE_STREAM)
bool cLuaAudioStream::update	() {
	// Number of processed buffers.
	ALint processed;

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

	while (processed--)
	{
		ALuint buffer;

		float curOffsetSamples, curOffsetSecs;

		alGetSourcef(source, AL_SAMPLE_OFFSET, &curOffsetSamples);

		ALint b;
		alGetSourcei(source, AL_BUFFER, &b);
		int freq;
		alGetBufferi(b, AL_FREQUENCY, &freq);
		curOffsetSecs = curOffsetSamples / freq;

		// Get a free buffer.
		alSourceUnqueueBuffers(source, 1, &buffer);

		float newOffsetSamples, newOffsetSecs;

		alGetSourcef(source, AL_SAMPLE_OFFSET, &newOffsetSamples);
		newOffsetSecs = newOffsetSamples / freq;

		offsetSamples += (curOffsetSamples - newOffsetSamples);
		offsetSeconds += (curOffsetSecs - newOffsetSecs);

		streamAtomic(buffer, decoder);
		alSourceQueueBuffers(source, 1, &buffer);
	}
	return true;
}

ALenum cLuaAudioStream::getFormat(int channels, int bits) const
{
	if (channels == 1 && bits == 8)
		return AL_FORMAT_MONO8;
	else if (channels == 1 && bits == 16)
		return AL_FORMAT_MONO16;
	else if (channels == 2 && bits == 8)
		return AL_FORMAT_STEREO8;
	else if (channels == 2 && bits == 16)
		return AL_FORMAT_STEREO16;
	else
		return 0;
}

int cLuaAudioStream::streamAtomic(ALuint buffer, cLuaAudioDecoder * d) {
	// Get more sound data.
	int decoded = d->decode();

	int fmt = getFormat(d->getChannels(), d->getBits());

	if (fmt != 0)
		alBufferData(buffer, fmt, d->getBuffer(), decoded, d->getSampleRate());
	
	//~ printf("cLuaAudioStream::streamAtomic %d\n",(int)buffer,(int)decoded);
	return decoded; // TODO : newly added samples ? see love2d::Source::streamAtomic
}

// ***** ***** ***** ***** ***** pure data wrapper

// ok: 8bit  unsigned
// ok: 16bit signed  (a bit quiet)
// fail-heavy : 8bit signed
// fail-sometimes : 16bit unsigned   with max=0xffff ,  ok with 0xCfff

/// dummy to keep code similar to love
class cLuaAudioDecoder_LibPD : public cLuaAudioDecoder { public:
	static const int BLOCK_SIZE = 64; // assert(64 == libpd_blocksize());
	static const int NUM_OUT_CHANNELS = 1;
	float inbuf[BLOCK_SIZE], outbuf[BLOCK_SIZE*NUM_OUT_CHANNELS];  // one input channel, two output channels, block size 64, one tick per buffer
	
	static const int BYTES_PER_SAMPLE = 2;
	static const unsigned short SAMPLE_MINVAL = 0;
	static const unsigned short SAMPLE_MAXVAL = 0xCfff;
	unsigned short mybuf[BLOCK_SIZE*NUM_OUT_CHANNELS];
	
	cLuaAudioDecoder_LibPD () {
		int i;
		for (i=0;i<sizeof(inbuf)/sizeof(float);++i) inbuf[i] = 0;
		for (i=0;i<sizeof(outbuf)/sizeof(float);++i) outbuf[i] = 0;
	}

	virtual void* getBuffer () const { return (void*)mybuf; }
	
	virtual int getChannels () { return NUM_OUT_CHANNELS; } // 1=mono 2=stereo
	virtual int getBits () { return BYTES_PER_SAMPLE * 8; } // 8bit / 16bit
	virtual int getSampleRate () { return DEFAULT_SAMPLE_RATE; } // 44k
	
	virtual int decode () {
		libpd_process_float(1, inbuf, outbuf);
		int num_samples = sizeof(outbuf)/sizeof(float);
		for (int i=0;i<num_samples;++i) { 
			float v = outbuf[i]; 
			v *= ((float)SAMPLE_MAXVAL);
				 if (v <= SAMPLE_MINVAL) mybuf[i] = SAMPLE_MINVAL;
			else if (v >= SAMPLE_MAXVAL) mybuf[i] = SAMPLE_MAXVAL; 
			else mybuf[i] = v;
		}
		return num_samples * BYTES_PER_SAMPLE;
	}
};

void pdprint(const char *s) {
  printf("pdprint: %s", s);
}

void pdnoteon(int ch, int pitch, int vel) {
  printf("pdnoteon: %d %d %d\n", ch, pitch, vel);
}

class cLuaPureDataPlayer { public:
	cLuaAudioStream*	pAudioStream;
	
	cLuaPureDataPlayer (cLuaAudio &luaAudio,const char* path_file,const char* path_folder) {
		// perpare audio
		cLuaAudioDecoder_LibPD* dec = new cLuaAudioDecoder_LibPD();
		
		// init pd
		int srate = dec->getSampleRate();
		libpd_printhook = (t_libpd_printhook) pdprint;
		libpd_noteonhook = (t_libpd_noteonhook) pdnoteon;
		libpd_init();
		libpd_init_audio(1, dec->getChannels(), srate);

		// compute audio    [; pd dsp 1(
		libpd_start_message(1); // one entry in list
		libpd_add_float(1.0f);
		libpd_finish_message("pd", "dsp");

		// open patch       [; pd open file folder(
		libpd_openfile(path_file, path_folder);

		//~ printf("libpd_blocksize=%d\n",(int)libpd_blocksize());

		// now run pd in loop for openal out
		pAudioStream = new cLuaAudioStream(dec);
		pAudioStream->setSource(luaAudio.makeSource());
		pAudioStream->playAtomic();
	}
	
	~cLuaPureDataPlayer () { delete pAudioStream; }
	
	void	update	() {
		pAudioStream->update();
	}
};


// ***** ***** ***** ***** ***** lua api

cLuaAudio luaAudio; // global 

static int L_helloworld (lua_State *L) {
	printf("lovepdaudio:hello world!\n");
	return 0;
}

static int L_test02 (lua_State *L) {
	const char* path_file = luaL_checkstring(L,1);
	const char* path_folder = ".";
	printf("lovepdaudio:test02 %s!\n",path_file);
	cLuaPureDataPlayer o(luaAudio,path_file,path_folder);
	while (true) { o.update(); }
	return 0;
}

static int L_CreatePureDataPlayer (lua_State *L) {
	const char* path_file = luaL_checkstring(L,1);
	const char* path_folder = ".";
	cLuaPureDataPlayer* o = new cLuaPureDataPlayer(luaAudio,path_file,path_folder);
	lua_pushlightuserdata(L,(void*)o);
	return 1;
}

static int L_PureDataPlayer_Update (lua_State *L) {
	cLuaPureDataPlayer* o = (cLuaPureDataPlayer*)lua_touserdata(L,1);
	o->update();
	return 0;
}

// ***** ***** ***** ***** ***** register


int LUA_API luaopen_lovepdaudio (lua_State *L) {
	printf("luaopen_lovepdaudio\n");
	struct luaL_reg funlist[] = {
		{"helloworld",		L_helloworld},
		{"test02",			L_test02},
		{"CreatePureDataPlayer",	L_CreatePureDataPlayer},
		{"PureDataPlayer_Update",	L_PureDataPlayer_Update},
		{NULL, NULL},
	};
	luaL_openlib (L, PROJECT_TABLENAME, funlist, 0);
	return 1;
}

/* test.lua :
require("lovepdaudio")
lovepdaudio.helloworld() 
*/
