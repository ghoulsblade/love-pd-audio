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
#include <string>

extern "C" {
	int LUA_API luaopen_lovepdaudio (lua_State *L);
}

// ***** ***** ***** ***** ***** utils

inline bool	LuaIsSet		(lua_State *L,int i) { return lua_gettop(L) >= i && !lua_isnil(L,i); }
	
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
	int num_buffers;
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
	cLuaAudioStream(cLuaAudioDecoder* decoder=0,int num_buffers=MAX_BUFFERS);
	/// destructor
	~cLuaAudioStream();
	
	void	setSource		(ALuint v);
	
	void	playAtomic		();
	bool	update			();
	int		streamAtomic	(ALuint buffer, cLuaAudioDecoder * d);
	ALenum	getFormat		(int channels, int bits) const;
};

// ***** ***** ***** ***** ***** cLuaAudioStream impl

cLuaAudioStream::cLuaAudioStream	(cLuaAudioDecoder* decoder,int num_buffers) : type(TYPE_STREAM), valid(false), source(0),
		pitch(1.0f), volume(1.0f), minVolume(0.0f),
		maxVolume(1.0f), referenceDistance(1.0f), rolloffFactor(1.0f), maxDistance(FLT_MAX),
		offsetSamples(0), offsetSeconds(0), decoder(decoder ? decoder : (new cLuaAudioDecoder_Dummy())){
	if (num_buffers > MAX_BUFFERS) num_buffers = MAX_BUFFERS;
	this->num_buffers = num_buffers;
	alGenBuffers(num_buffers, buffers);
}

cLuaAudioStream::~cLuaAudioStream	() {
	alDeleteBuffers(num_buffers, buffers);
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
	unsigned short* mybuf;
	int blocks_per_tick;
	
	~cLuaAudioDecoder_LibPD () { delete[] mybuf; }
	cLuaAudioDecoder_LibPD (int blocks_per_tick=1) : blocks_per_tick(blocks_per_tick) {
		mybuf = new unsigned short[blocks_per_tick*BLOCK_SIZE*NUM_OUT_CHANNELS];
		int i;
		for (i=0;i<sizeof(inbuf)/sizeof(float);++i) inbuf[i] = 0;
		for (i=0;i<sizeof(outbuf)/sizeof(float);++i) outbuf[i] = 0;
	}

	virtual void* getBuffer () const { return (void*)mybuf; }
	
	virtual int getChannels () { return NUM_OUT_CHANNELS; } // 1=mono 2=stereo
	virtual int getBits () { return BYTES_PER_SAMPLE * 8; } // 8bit / 16bit
	virtual int getSampleRate () { return DEFAULT_SAMPLE_RATE; } // 44k
	
	virtual int decode () {
		int num_samples_per_block = sizeof(outbuf)/sizeof(float);
		for (int ib=0;ib<blocks_per_tick;++ib) {
			int ioff = ib*num_samples_per_block;
			libpd_process_float(1, inbuf, outbuf);
			for (int i=0;i<num_samples_per_block;++i) { 
				float v = outbuf[i]; 
				v *= ((float)SAMPLE_MAXVAL);
					 if (v <= SAMPLE_MINVAL) mybuf[i+ioff] = SAMPLE_MINVAL;
				else if (v >= SAMPLE_MAXVAL) mybuf[i+ioff] = SAMPLE_MAXVAL; 
				else mybuf[i+ioff] = v;
			}
		}
		return blocks_per_tick * num_samples_per_block * BYTES_PER_SAMPLE;
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
	
	cLuaPureDataPlayer (cLuaAudio &luaAudio,const char* path_file,const char* path_folder,int delay_msec=50,int num_buffers=4) {
		// calc delay params
		if (num_buffers < 2) num_buffers = 2;
		int msec_per_tick = delay_msec / num_buffers;
		int srate = cLuaAudioDecoder_LibPD::DEFAULT_SAMPLE_RATE;
		int samples_per_tick = msec_per_tick * srate / 1000;
		int samples_per_block = libpd_blocksize();
		int blocks_per_tick = samples_per_tick / samples_per_block;
		if (blocks_per_tick < 1) blocks_per_tick = 1;
		
		printf("delay_msec = %d\n",delay_msec);
		printf("num_buffers = %d\n",num_buffers);
		printf("blocks_per_tick = %d\n",blocks_per_tick);
		
		// init decoder
		cLuaAudioDecoder_LibPD* dec = new cLuaAudioDecoder_LibPD(blocks_per_tick);
		//~ int srate = dec->getSampleRate();
		
		// init pd
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
		pAudioStream = new cLuaAudioStream(dec,num_buffers);
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
	int delay_msec = LuaIsSet(L,2) ? luaL_checkint(L,2) : 50;
	int num_buffers = LuaIsSet(L,3) ? luaL_checkint(L,3) : 4;
	cLuaPureDataPlayer* o = new cLuaPureDataPlayer(luaAudio,path_file,path_folder,delay_msec,num_buffers);
	lua_pushlightuserdata(L,(void*)o);
	return 1;
}

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
	// see https://github.com/libpd/libpd/wiki/libpd
	
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
	QUICKWRAP_GLOBALFUN_1RET(PushLUData	,libpd_bind		,(ParamString(L)));
	QUICKWRAP_GLOBALFUN_VOID(			 libpd_unbind	,(ParamLUData(L)));
	// TODO: luacallback for  libpd_printhook,libpd_banghook,libpd_floathook,libpd_symbolhook,libpd_listhook,libpd_messagehook
	
	// Accessing arrays in Pd 
	//~ int libpd_arraysize(const char *name)
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
	
	RegisterLibPD(L);
	
	
	return 1;
}

/* test.lua :
require("lovepdaudio")
lovepdaudio.helloworld() 
*/
