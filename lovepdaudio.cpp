// myhello.cpp  ->  myhello.dll (msvc6 : dll example project, select c++:codegen:"multithreaded dll" and add luabin include&lib paths)
//~ #include <openal.h>
// win:vc6 : add openal.lib ?  to linker (and paths accordingly)

#include <stdio.h>
extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
}
#define PROJECT_TABLENAME "lovepdaudio"

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

// ***** ***** ***** ***** ***** cLuaAudioStream

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

/// dummy to keep code similar to love
class cLuaAudioDecoder { public:
	static const int DEFAULT_SAMPLE_RATE = 44100;
	unsigned char mybuf[10*1024];

	cLuaAudioDecoder () {
		for (int i=0;i<sizeof(mybuf)/(getBits()/8);++i) mybuf[i] = (i/1) % 255;
	}
	
	bool isFinished () { return false; }
	void* getBuffer () const { return (void*)mybuf; }
	int decode () { return sizeof(mybuf)/(getBits()/8); } // TODO
	int getChannels () { return 1; } // mono
	int getBits () { return 8; } // 8bit
	int getSampleRate () { return DEFAULT_SAMPLE_RATE; } // 44k

};

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
	cLuaAudioStream();
	/// destructor
	~cLuaAudioStream();
	
	void	setSource		(ALuint v);
	
	void	playAtomic		();
	bool	update			();
	int		streamAtomic	(ALuint buffer, cLuaAudioDecoder * d);
	ALenum	getFormat		(int channels, int bits) const;
};

// ***** ***** ***** ***** ***** cLuaAudioStream impl

cLuaAudioStream::cLuaAudioStream	() : type(TYPE_STREAM), valid(false), source(0),
		pitch(1.0f), volume(1.0f), minVolume(0.0f),
		maxVolume(1.0f), referenceDistance(1.0f), rolloffFactor(1.0f), maxDistance(FLT_MAX),
		offsetSamples(0), offsetSeconds(0), decoder(new cLuaAudioDecoder()){
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

// ***** ***** ***** ***** ***** lua api

static int L_helloworld (lua_State *L) {
	printf("lovepdaudio:hello world!\n");
	return 0;
}

void MySleep (int iSeconds) {
	#ifndef WIN32
		sleep(iSeconds);
	#else
		Sleep(iSeconds*1000); // takes milliseconds
	#endif
}

static int L_test01 (lua_State *L) {
	printf("lovepdaudio:test01!\n");
	cLuaAudio luaAudio;
	cLuaAudioStream o;
	o.setSource(luaAudio.makeSource());
	o.playAtomic();
	for (int i=0;i<100000;++i) { o.update(); MySleep(1); }
	return 0;
}

// ***** ***** ***** ***** ***** register

int LUA_API luaopen_lovepdaudio (lua_State *L) {
	printf("luaopen_lovepdaudio\n");
	struct luaL_reg funlist[] = {
		{"helloworld",		L_helloworld},			
		{"test01",		L_test01},			
		{NULL, NULL},
	};
	luaL_openlib (L, PROJECT_TABLENAME, funlist, 0);
	return 1;
}

/* test.lua :
require("lovepdaudio")
lovepdaudio.helloworld() 
*/
