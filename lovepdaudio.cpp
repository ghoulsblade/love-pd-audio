// myhello.cpp  ->  myhello.dll (msvc6 : dll example project, select c++:codegen:"multithreaded dll" and add luabin include&lib paths)
//~ #include <openal.h>
// win:vc6 : add openal.lib ?  to linker (and paths accordingly)
// http://connect.creativelabs.com/openal/Documentation/OpenAL%201.1%20Specification.htm
// http://connect.creativelabs.com/openal/Documentation/OpenAL_Programmers_Guide.pdf

/*
// notes visual studio
PLATFORM_CFLAGS = -DWINVER=0x502 -DWIN32 -D_WIN32 -DPD_INTERNAL -O3 \
CFLAGS = -DPD -DHAVE_UNISTD_H -DUSEAPI_DUMMY -I./pure-data/src -I./libpd_wrapper

-DWINVER=0x502 -DWIN32 -D_WIN32 -DPD_INTERNAL -DPD -DHAVE_UNISTD_H -DUSEAPI_DUMMY
/D "WINVER=0x502" /D "WIN32" /D "_WIN32" /D "PD_INTERNAL" /D "PD" /D "HAVE_UNISTD_H" /D "USEAPI_DUMMY"

-DWINVER=0x400 -DWIN32 -D_WIN32 -DPD_INTERNAL -DPD -DHAVE_UNISTD_H -DUSEAPI_DUMMY
/D "WINVER=0x400" /D "WIN32" /D "_WIN32" /D "PD_INTERNAL" /D "PD" /D "HAVE_UNISTD_H" /D "USEAPI_DUMMY"


msvc6 2012-09-09
/nologo /MD /W3 /GX /O2 /I "../include" /D "WIN32" /D "_WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSVC6LOVEPDAUDIO_EXPORTS" /D WINVER=0x400 /D "PD_INTERNAL" /D "PD" /D "HAVE_UNISTD_H" /D "USEAPI_DUMMY" /Fo"Release/" /Fd"Release/" /FD /c 
TODO: compile libpd with -DWINVER=0x400  :

Creating library file: libs/libpd.lib
pure-data/src/s_loader.o:s_loader.c:(.text+0x26f): undefined reference to `SetDllDirectory'
pure-data/src/s_loader.o:s_loader.c:(.text+0x2b3): undefined reference to `SetDllDirectory'
pure-data/src/s_loader.o:s_loader.c:(.text+0x50e): undefined reference to `SetDllDirectory'
collect2: ld returned 1 exit status
make: *** [libs/libpd.dll] Error 1

solution? added pure-data/src/s_loader.c
		#if WINVER < 0x0502
		printf("warning:SetDllDirectory not available\n");
		#else
		...
		#endif

still crashes...

*/

#define USE_OPENAL
#define USE_LIBPD    // libpd.lib 

#define PROJECT_TABLENAME "lovepdaudio"

#include <stdlib.h> // exit
#include <stdio.h>
#include <stdarg.h>
extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
}

// headers
#ifdef USE_LIBPD
extern "C" {
#include "z_libpd.h"
}
#endif
#include <float.h>
#include <string>

// Platform
#if defined(WIN32) || defined(_WIN32)
#	define MY_WINDOWS 1
#endif
#if defined(linux) || defined(__linux) || defined(__linux__)
#	define MY_LINUX 1
#endif
#if defined(__APPLE__)
#	define MY_MACOSX 1
#endif
#if defined(macintosh)
#	define MY_MACOS 1
#endif

// lua
#ifdef LUA_API
#undef LUA_API
#endif
#ifdef MY_WINDOWS
#define LUA_API __declspec(dllexport)
#else
#define LUA_API
#endif

// var arg printf
#ifdef MY_WINDOWS
#define vsnprintf     _vsnprintf
#endif

// sleep
#ifdef MY_WINDOWS
#else
//~ #include <unistd.h> // Sleep
#endif

// OpenAL
#ifdef USE_OPENAL
#ifdef MY_MACOSX
#include <OpenAL/alc.h>
#include <OpenAL/al.h>
#include <OpenAL/alext.h> // ALC_DEFAULT_ALL_DEVICES_SPECIFIER
#else
#include <AL/alc.h>
#include <AL/al.h>
#include <AL/alext.h> // ALC_DEFAULT_ALL_DEVICES_SPECIFIER
#endif
#endif


// ***** ***** ***** ***** ***** prototypes

lua_State* gMainLuaState = 0;

extern "C" {
	int LUA_API luaopen_lovepdaudio (lua_State *L);
}

// ***** ***** ***** ***** ***** utils

inline bool	LuaIsSet		(lua_State *L,int i) { return lua_gettop(L) >= i && !lua_isnil(L,i); }
	
// ***** ***** ***** ***** ***** cLuaAudio

#ifdef USE_OPENAL
#define cLuaAudio_NUM_SOURCES 4
class cLuaAudio { public:
	// The OpenAL device.
	ALCdevice * device;

	// The OpenAL capture device (microphone).
	ALCdevice * capture;

	// The OpenAL context.
	ALCcontext * context;
	
	static const int NUM_SOURCES;
	// OpenAL sources
	ALuint sources[cLuaAudio_NUM_SOURCES];
	
	ALuint makeSource () { return sources[0]; }

	/// constructor
	cLuaAudio () : device(0),capture(0),context(0) {
		printf("cLuaAudio 00\n");
		const char* szDeviceName = alcGetString(NULL,ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
		printf("cLuaAudio szDeviceName='%s'\n",szDeviceName);

		// Passing zero for default device.
		printf("cLuaAudio 01\n");
		device = alcOpenDevice(szDeviceName);
		printf("cLuaAudio 01 done.\n");


		if (device == 0) { fail("Could not open device."); return; }

		printf("cLuaAudio 02\n");
		context = alcCreateContext(device, 0);

		if (context == 0) { fail("Could not create context."); return; }

		printf("cLuaAudio 03\n");
		alcMakeContextCurrent(context);

		if (alcGetError(device) != ALC_NO_ERROR) { fail("Could not make context current."); return; }
		
		
		// Generate sources.
		printf("cLuaAudio 04\n");
		alGenSources(NUM_SOURCES, sources);

		if (alGetError() != AL_NO_ERROR) { fail("Could not generate sources."); return; }
		
		printf("cLuaAudio 05\n");
		//~ printf("cLuaAudio init ok\n");
	}
	
	~cLuaAudio () {
		printf("cLuaAudio destructor!\n");
		// Free all sources.
		alDeleteSources(NUM_SOURCES, sources);
		
		
		alcMakeContextCurrent(0);
		alcDestroyContext(context);
		//if (capture) alcCaptureCloseDevice(capture);
		alcCloseDevice(device);
	}
	
	void fail (const char* txt) { printf("cLuaAudio:fail %s\n",txt); }
};
const int cLuaAudio::NUM_SOURCES = cLuaAudio_NUM_SOURCES;
#else
class cLuaAudio { public:
	cLuaAudio () {}
};
#endif

// ***** ***** ***** ***** ***** cLuaAudioDecoder

#ifdef USE_OPENAL
/// base class
class cLuaAudioDecoder { public:
	static const int DEFAULT_SAMPLE_RATE;

	cLuaAudioDecoder () {}
	
	virtual bool isFinished () { return false; }
	virtual void* getBuffer () const { return 0; } ///< override me
	virtual int decode () { return 0; } ///< override me
	virtual int getChannels () { return 1; } // mono
	virtual int getBits () { return 8; } // 8bit
	virtual int getSampleRate () { return DEFAULT_SAMPLE_RATE; } // 44k
};
const int cLuaAudioDecoder::DEFAULT_SAMPLE_RATE = 44100;

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
#endif

// ***** ***** ***** ***** ***** cLuaAudioStream

#ifdef USE_OPENAL
/// based on love2d::Source
#define cLuaAudioStream_MAX_BUFFERS 32
class cLuaAudioStream { public:
	typedef enum { TYPE_STREAM, TYPE_STATIC } eType;
	eType type;
	cLuaAudioDecoder* decoder;
	
	ALuint source;
	bool valid;
	static const unsigned int MAX_BUFFERS;
	int num_buffers;
	ALuint buffers[cLuaAudioStream_MAX_BUFFERS];
	
	
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
	cLuaAudioStream(cLuaAudioDecoder* decoder=0,int num_buffers=MAX_BUFFERS);
	/// destructor
	~cLuaAudioStream();
	
	void	setSource		(ALuint v);
	bool	isStopped() const;
	
	void	resumePlayback	();
	void	stopAtomic		();
	void	rewindAtomic	();
	void	playAtomic		();
	bool	update			();
	int		streamAtomic	(ALuint buffer, cLuaAudioDecoder * d);
	ALenum	getFormat		(int channels, int bits) const;
};
const unsigned int cLuaAudioStream::MAX_BUFFERS = cLuaAudioStream_MAX_BUFFERS;
#else
class cLuaAudioStream { public:
	cLuaAudioStream() {}
};
#endif

// ***** ***** ***** ***** ***** cLuaAudioStream impl

#ifdef USE_OPENAL
cLuaAudioStream::cLuaAudioStream	(cLuaAudioDecoder* decoder,int num_buffers) : type(TYPE_STREAM), valid(false), source(0),
		pitch(1.0f), volume(1.0f), minVolume(0.0f),
		maxVolume(1.0f), referenceDistance(1.0f), rolloffFactor(1.0f), maxDistance(FLT_MAX),
		offsetSamples(0), offsetSeconds(0), decoder(decoder ? decoder : (new cLuaAudioDecoder_Dummy())){
	if (num_buffers > MAX_BUFFERS) num_buffers = MAX_BUFFERS;
	this->num_buffers = num_buffers;
	alGenBuffers(num_buffers, buffers);
}

cLuaAudioStream::~cLuaAudioStream	() {
	printf("cLuaAudioStream destructor!\n");
	alDeleteBuffers(num_buffers, buffers);
}

void cLuaAudioStream::setSource (ALuint v) {
	source = v;
	valid = true;
}

bool cLuaAudioStream::isStopped() const {
	if (valid)
	{
		ALenum state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		return (state == AL_STOPPED);
	}

	return true;
}

void	cLuaAudioStream::resumePlayback	() {
	printf("cLuaAudioStream::resumePlayback (update wasn't called fast enough, increase delay?)\n");
	ALuint old_source = source;
	stopAtomic();
	rewindAtomic();
	setSource(old_source);
	playAtomic();
}

void cLuaAudioStream::rewindAtomic()
{
	offsetSamples = 0;
	offsetSeconds = 0;
}

void cLuaAudioStream::stopAtomic()
{
	if (valid)
	{
		if (type == TYPE_STATIC)
		{
			alSourceStop(source);
		}
		else if (type == TYPE_STREAM)
		{
			alSourceStop(source);
			int queued = 0;
			alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

			while (queued--)
			{
				ALuint buffer;
				alSourceUnqueueBuffers(source, 1, &buffer);
			}
		}
		alSourcei(source, AL_BUFFER, AL_NONE);
	}
	valid = false;
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

		for (unsigned int i = 0; i < num_buffers; i++)
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
#endif

// ***** ***** ***** ***** ***** cLuaAudioDecoder_LibPD

// ok: 8bit  unsigned
// ok: 16bit signed  (a bit quiet)
// fail-heavy : 8bit signed
// fail-sometimes : 16bit unsigned   with max=0xffff ,  ok with 0xCfff

#ifdef USE_OPENAL
#ifdef USE_LIBPD
#define DECODER_LIBPD_VALID
/// dummy to keep code similar to love


#define cLuaAudioDecoder_LibPD_BLOCK_SIZE 64
#define cLuaAudioDecoder_LibPD_NUM_OUT_CHANNELS 1

class cLuaAudioDecoder_LibPD : public cLuaAudioDecoder { public:
	static const int BLOCK_SIZE; // assert(64 == libpd_blocksize());
	static const int NUM_OUT_CHANNELS;
	short inbuf[cLuaAudioDecoder_LibPD_BLOCK_SIZE]; // one input channel, two output channels, block size 64, one tick per buffer
	
	static const int BYTES_PER_SAMPLE;
	short* mybuf;
	int blocks_per_tick;
	
	~cLuaAudioDecoder_LibPD () { delete[] mybuf; }
	cLuaAudioDecoder_LibPD (int blocks_per_tick=1) : blocks_per_tick(blocks_per_tick) {
		if (libpd_blocksize() != BLOCK_SIZE) printf("warning, unexpected blocksize %d, should be %d\n",(int)libpd_blocksize(),(int)BLOCK_SIZE);
		mybuf = new short[blocks_per_tick*BLOCK_SIZE*NUM_OUT_CHANNELS];
		{ for (int i=0;i<cLuaAudioDecoder_LibPD_BLOCK_SIZE;++i) inbuf[i] = 0; }
	}

	virtual void* getBuffer () const { return (void*)mybuf; }
	
	virtual int getChannels () { return NUM_OUT_CHANNELS; } // 1=mono 2=stereo
	virtual int getBits () { return BYTES_PER_SAMPLE * 8; } // 8bit / 16bit
	virtual int getSampleRate () { return DEFAULT_SAMPLE_RATE; } // 44k
	
	virtual int decode () {
		int num_samples_per_block = cLuaAudioDecoder_LibPD_BLOCK_SIZE*cLuaAudioDecoder_LibPD_NUM_OUT_CHANNELS;
		for (int ib=0;ib<blocks_per_tick;++ib) {
			int ioff = ib*(BLOCK_SIZE*NUM_OUT_CHANNELS);
			libpd_process_short(1,inbuf,&mybuf[ioff]); // warning: no clipping of samples outside [-1, 1]
		}
		return blocks_per_tick * num_samples_per_block * BYTES_PER_SAMPLE;
	}
};
const int cLuaAudioDecoder_LibPD::BLOCK_SIZE = cLuaAudioDecoder_LibPD_BLOCK_SIZE; // assert(64 == libpd_blocksize());
const int cLuaAudioDecoder_LibPD::NUM_OUT_CHANNELS = cLuaAudioDecoder_LibPD_NUM_OUT_CHANNELS;

const int cLuaAudioDecoder_LibPD::BYTES_PER_SAMPLE = 2;
#endif
#endif

#ifndef DECODER_LIBPD_VALID
class cLuaAudioDecoder_LibPD { public:
	static const int DEFAULT_SAMPLE_RATE;
	static const int NUM_OUT_CHANNELS;
	cLuaAudioDecoder_LibPD (int blocks_per_tick) {}
	virtual int getChannels () { return NUM_OUT_CHANNELS; } // 1=mono 2=stereo
};
const int cLuaAudioDecoder_LibPD::DEFAULT_SAMPLE_RATE = 44100;
const int cLuaAudioDecoder_LibPD::NUM_OUT_CHANNELS = 1;
#endif

// ***** ***** ***** ***** ***** cLuaPureDataPlayer

void lua_libpd_hook (const char* eventname,const char* fmt,...);

// note: called during step?
void	callback_libpd_banghook		(const char *s) { lua_libpd_hook("banghook","%s", s); }
void	callback_libpd_printhook	(const char *s) { lua_libpd_hook("printhook","%s", s); }
void	callback_libpd_floathook	(const char *s,float v) { lua_libpd_hook("floathook","%s %f",s,v); }
void	callback_libpd_symbolhook	(const char *s,const char* v) { lua_libpd_hook("symbolhook","%s %s",s,v); }
void	callback_libpd_noteonhook	(int ch, int pitch, int vel) { lua_libpd_hook("noteonhook","%d %d %d", ch, pitch, vel); }

// TODO?  typedef (*t_libpd_listhook)(const char *source, int argc, t_atom *argv)
// TODO?  typedef (*t_libpd_messagehook)(const char *source, const char *symbol, int argc, t_atom *argv)


class cLuaPureDataPlayer { public:
	cLuaAudioStream*	pAudioStream;
	
	cLuaPureDataPlayer (cLuaAudio &luaAudio,const char* path_file,const char* path_folder,int delay_msec=50,int num_buffers=4) :
		pAudioStream(0) {
		#ifdef USE_LIBPD
		// calc delay params
		if (num_buffers < 2) num_buffers = 2;
		int msec_per_tick = delay_msec / num_buffers;
		int srate = cLuaAudioDecoder_LibPD::DEFAULT_SAMPLE_RATE;
		int samples_per_tick = msec_per_tick * srate / 1000;
		int samples_per_block = libpd_blocksize();
		int blocks_per_tick = samples_per_tick / samples_per_block;
		if (blocks_per_tick < 1) blocks_per_tick = 1;
		
		printf("lovepdaudio.delay_msec = %d\n",delay_msec);
		printf("lovepdaudio.num_buffers = %d\n",num_buffers);
		printf("lovepdaudio.blocks_per_tick = %d\n",blocks_per_tick);
		
		// init decoder
		cLuaAudioDecoder_LibPD* dec = new cLuaAudioDecoder_LibPD(blocks_per_tick);
		//~ int srate = dec->getSampleRate();
		
		// init pd
		int nInputs = 0;
		int nOutputs = dec->getChannels();
		libpd_init_audio(nInputs, nOutputs, srate);

		// compute audio    [; pd dsp 1(
		libpd_start_message(1); // one entry in list
		libpd_add_float(1.0f);
		libpd_finish_message("pd", "dsp");

		// open patch       [; pd open file folder(
		libpd_openfile(path_file, path_folder);

		//~ printf("libpd_blocksize=%d\n",(int)libpd_blocksize());

		// now run pd in loop for openal out
		#ifdef USE_OPENAL
		pAudioStream = new cLuaAudioStream(dec,num_buffers);
		pAudioStream->setSource(luaAudio.makeSource());
		pAudioStream->playAtomic();
		#endif
		#endif
	}
	
	~cLuaPureDataPlayer () { printf("cLuaPureDataPlayer destructor!\n"); if (pAudioStream) delete pAudioStream; }
	
	void	update	() {
		#ifdef USE_LIBPD
		#ifdef USE_OPENAL
		if (pAudioStream->isStopped()) pAudioStream->resumePlayback();
		pAudioStream->update();
		#endif
		#endif
	}
};


// ***** ***** ***** ***** ***** lua_libpd_hook

/// also adds a traceback to the error message in case of an error, better than a plain lua_call
/// nret=-1 for unlimited
/// don't use directly, stack has to be prepared and cleaned up
int 	PCallWithErrFuncWrapper (lua_State* L,int narg, int nret) {
	int errfunc = 0;
	return lua_pcall(L, narg, (nret==-1) ? LUA_MULTRET : nret, errfunc);
}

void	LuaPCall_StrStr	(lua_State *L,const char* func,const char* a,const char* b) {
	lua_getglobal(L, func); // get function
	if (lua_isnil(L,1)) {
		lua_pop(L,1);
		//~ fprintf(stderr,"lua: function `%s' not found\n",func);
	} else {
		int narg = 0, nres = 0;
		++narg; lua_pushstring(L, a);
		++narg; lua_pushstring(L, b);
		if (PCallWithErrFuncWrapper(L,narg, nres) != 0) {
			fprintf(stderr,"lua: error running function `%s': %s\n",func, lua_tostring(L, -1));
		} else {
			//~ gboolean res = lua_toboolean(L,-1);
			//~ if (nres > 0) lua_pop(L, nres);
			//~ if (res) return TRUE;
		}
	}
}


#define LUA_LIBPD_HOOK__MAX_PARAM_TEXT_LEN 1024
#define LUA_LIBPD_HOOK__LUA_CALLBACK_NAME "libpdhook"	// name of the lua function that will be called
void lua_libpd_hook (const char* eventname,const char* fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	char mybuf[LUA_LIBPD_HOOK__MAX_PARAM_TEXT_LEN];
	mybuf[0] = 0;
	vsnprintf(mybuf,sizeof(mybuf)-1,fmt,ap);
	//~ printf("%s\n",mybuf);
	//~ std::string s(mybuf);
	lua_State *L = gMainLuaState;
	LuaPCall_StrStr(L,LUA_LIBPD_HOOK__LUA_CALLBACK_NAME,eventname,mybuf);
	va_end(ap);
}

// ***** ***** ***** ***** ***** lua api

//cLuaAudio luaAudio; // global 
cLuaAudio* gpLuaAudio = 0; // global 
cLuaAudio& GetMyAudio	() {
	if (!gpLuaAudio) { printf("attempt to access uninitialized gpLuaAudio\n"); exit(0); }
	return *gpLuaAudio;
}

static int L_helloworld (lua_State *L) {
	printf("lovepdaudio:hello world!\n");
	return 0;
}

static int L_test02 (lua_State *L) {
	const char* path_file = luaL_checkstring(L,1);
	const char* path_folder = ".";
	printf("lovepdaudio:test02 %s!\n",path_file);
	cLuaPureDataPlayer o(GetMyAudio(),path_file,path_folder);
	while (true) { o.update(); }
	return 0;
}

/// for lua: player = lovepdaudio.CreatePureDataPlayer(path_file,path_folder=".",delay_msec=50,num_buffers=4)
static int L_CreatePureDataPlayer (lua_State *L) {
	const char* path_file = luaL_checkstring(L,1);
	const char* path_folder = LuaIsSet(L,2) ? luaL_checkstring(L,2) : ".";
	int delay_msec = LuaIsSet(L,3) ? luaL_checkint(L,3) : 100;
	int num_buffers = LuaIsSet(L,4) ? luaL_checkint(L,4) : 4;
	cLuaPureDataPlayer* o = new cLuaPureDataPlayer(GetMyAudio(),path_file,path_folder,delay_msec,num_buffers);
	o->update();
	lua_pushlightuserdata(L,(void*)o);
	return 1;
}

/// for lua: lovepdaudio.PureDataPlayer_Update(player)
static int L_PureDataPlayer_Update (lua_State *L) {
	cLuaPureDataPlayer* o = (cLuaPureDataPlayer*)lua_touserdata(L,1);
	o->update();
	return 0;
}

	
// ***** ***** ***** ***** ***** utils
	
#define LUABIND_QUICKWRAP_STATIC(methodname,code) \
	{ 	class cTempClass { public: \
			static int LUABIND_ ## methodname (lua_State *L) { code return 0; }\
		}; \
		lua_register(L,#methodname,&cTempClass::LUABIND_ ## methodname); \
	}
	
#define QUICKWRAP_GLOBALFUN_1RET(retfun,name,params) LUABIND_QUICKWRAP_STATIC(name, return retfun(L,name params); )
#define QUICKWRAP_GLOBALFUN_VOID(name,params) 	LUABIND_QUICKWRAP_STATIC(name, name params ; ) // return void
	
const char*	ParamString	(lua_State *L,int i=1) { return std::string(luaL_checkstring(L,i)).c_str(); }
float		ParamFloat	(lua_State *L,int i=1) { return luaL_checknumber(L,i); }
float		ParamInt	(lua_State *L,int i=1) { return luaL_checkint(L,i); }
void*		ParamLUData	(lua_State *L,int i=1) { return lua_touserdata(L,i); }

int			PushInt		(lua_State *L,int v) { lua_pushinteger(L,v); return 1; }
int			PushLUData	(lua_State *L,void* v) { lua_pushlightuserdata(L,v); return 1; }

// ***** ***** ***** ***** ***** RegisterLibPD
	
void RegisterLibPD (lua_State *L) {
	#ifdef USE_LIBPD
	// see https://github.com/libpd/libpd/wiki/libpd
	printf("RegisterLibPD 01\n");

	libpd_init();
	
	printf("RegisterLibPD 02\n");

	// init pd
	/*
	#ifndef MY_WINDOWS
	libpd_banghook		= (t_libpd_banghook)	callback_libpd_banghook;
	libpd_printhook		= (t_libpd_printhook)	callback_libpd_printhook;
	libpd_floathook		= (t_libpd_floathook)	callback_libpd_floathook;
	libpd_symbolhook	= (t_libpd_symbolhook)	callback_libpd_symbolhook;
	libpd_noteonhook	= (t_libpd_noteonhook)	callback_libpd_noteonhook;
	#endif
	*/

	printf("RegisterLibPD 03\n");

	
	// Initializing Pd
	QUICKWRAP_GLOBALFUN_VOID(			 libpd_clear_search_path		,());
	QUICKWRAP_GLOBALFUN_VOID(			 libpd_add_to_search_path		,(ParamString(L)));
	
	// Opening patches
	QUICKWRAP_GLOBALFUN_1RET(PushLUData	,libpd_openfile			,(ParamString(L),ParamString(L,2)));
	QUICKWRAP_GLOBALFUN_VOID(			 libpd_closefile		,(ParamLUData(L)));
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_getdollarzero	,(ParamLUData(L)));
	
	// Audio processing with Pd
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_blocksize		,());
	
	// Sending messages to Pd
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_bang				,(ParamString(L)));
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_float			,(ParamString(L),ParamFloat(L,2)));
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_symbol			,(ParamString(L),ParamString(L,2)));
	
	// Sending compound messages: Simple approach for wrapping
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_start_message	,(ParamInt(L)));
	QUICKWRAP_GLOBALFUN_VOID(			 libpd_add_float		,(ParamFloat(L)));
	QUICKWRAP_GLOBALFUN_VOID(			 libpd_add_symbol		,(ParamString(L)));
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_finish_list		,(ParamString(L)));
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_finish_message	,(ParamString(L),ParamString(L,2)));
	
	// Sending compound messages: Flexible approach   : needs t_atom
	
	// Receiving messages from Pd
	QUICKWRAP_GLOBALFUN_1RET(PushLUData	,libpd_bind			,(ParamString(L)));
	QUICKWRAP_GLOBALFUN_VOID(			 libpd_unbind		,(ParamLUData(L)));
	// see also libpd_printhook,libpd_banghook,libpd_floathook,libpd_symbolhook,libpd_listhook,libpd_messagehook
	
	// Accessing arrays in Pd 
	QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_arraysize	,(ParamString(L)));
	//~ int libpd_read_array(float *dest, const char *src, int offset, int n)
	//~ int libpd_write_array(const char *dest, int offset, float *src, int n)
	// TODO: needs size check, buffer alloc and free (std::string?std::vector?)
	
	// MIDI support in libpd
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_noteon			,(ParamInt(L),ParamInt(L,2),ParamInt(L,3)));
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_controlchange	,(ParamInt(L),ParamInt(L,2),ParamInt(L,3)));
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_programchange	,(ParamInt(L),ParamInt(L,2)));
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_pitchbend		,(ParamInt(L),ParamInt(L,2)));
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_aftertouch		,(ParamInt(L),ParamInt(L,2)));
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_polyaftertouch	,(ParamInt(L),ParamInt(L,2),ParamInt(L,3)));
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_midibyte			,(ParamInt(L),ParamInt(L,2)));
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_sysex			,(ParamInt(L),ParamInt(L,2)));
    QUICKWRAP_GLOBALFUN_1RET(PushInt	,libpd_sysrealtime		,(ParamInt(L),ParamInt(L,2)));
	
	// todo: receive midi ?  libpd_noteonhook, libpd_controlchangehook...
	#endif
	
	printf("RegisterLibPD 04\n");
}

// ***** ***** ***** ***** ***** register

int LUA_API luaopen_lovepdaudio (lua_State *L) {
	gMainLuaState = L;
	gpLuaAudio = new cLuaAudio(); // global 

	//~ printf("luaopen_lovepdaudio\n");
	struct luaL_reg funlist[] = {
		{"helloworld",		L_helloworld},
		{"test02",			L_test02},
		{"CreatePureDataPlayer",	L_CreatePureDataPlayer},
		{"PureDataPlayer_Update",	L_PureDataPlayer_Update},
		{NULL, NULL},
	};
	luaL_openlib (L, PROJECT_TABLENAME, funlist, 0);
	
	RegisterLibPD(L);
	
	
	return 1;
}

/* test.lua :
require("lovepdaudio")
lovepdaudio.helloworld() 
*/
