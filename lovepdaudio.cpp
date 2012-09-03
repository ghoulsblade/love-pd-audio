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

/// dummy to keep code similar to love
class cLuaAudioDecoder { public:
	bool isFinished () { return false; }
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
	
	void	playAtomic		();
	bool	update			();
	int		streamAtomic	(ALuint buffer, cLuaAudioDecoder * d);
};

// ***** ***** ***** ***** ***** cLuaAudioStream impl

cLuaAudioStream::cLuaAudioStream	() : type(TYPE_STREAM), valid(false),
		pitch(1.0f), volume(1.0f), minVolume(0.0f),
		maxVolume(1.0f), referenceDistance(1.0f), rolloffFactor(1.0f), maxDistance(FLT_MAX),
		offsetSamples(0), offsetSeconds(0), decoder(new cLuaAudioDecoder()){
	alGenBuffers(MAX_BUFFERS, buffers);
}

cLuaAudioStream::~cLuaAudioStream	() {
	alDeleteBuffers(MAX_BUFFERS, buffers);
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

int cLuaAudioStream::streamAtomic(ALuint buffer, cLuaAudioDecoder * d) {
	return 50; // TODO : newly added samples ? see love2d::Source::streamAtomic
}

// ***** ***** ***** ***** ***** lua api

static int L_helloworld (lua_State *L) {
	printf("lovepdaudio:hello world!\n");
	return 0;
}

static int L_test01 (lua_State *L) {
	printf("lovepdaudio:test01!\n");
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
