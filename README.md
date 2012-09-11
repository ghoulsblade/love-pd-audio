DOWNLOAD
========

* [Win DLL lovepdaudio.dll](https://github.com/ghoulsblade/love-pd-audio/raw/master/bin/lovepdaudio.dll) (needs 32bit love2d binaries to work under 64bit win)
* win also needs [libpd.dll](https://github.com/ghoulsblade/love-pd-audio/raw/master/bin/libpd.dll)
* win also needs [pthreadGC2.dll](https://github.com/ghoulsblade/love-pd-audio/raw/master/bin/pthreadGC2.dll)
* for pure lua without löve win also needs [OpenAL32.dll](https://github.com/ghoulsblade/love-pd-audio/raw/master/bin/OpenAL32.dll)

* [Linux 32Bit lovepdaudio.so.32](https://github.com/ghoulsblade/love-pd-audio/raw/master/bin/lovepdaudio.so.32) (rename to lovepdaudio.so) (Ubuntu/Debian)
* [Linux 64Bit lovepdaudio.so.64](https://github.com/ghoulsblade/love-pd-audio/raw/master/bin/lovepdaudio.so.64) (rename to lovepdaudio.so) (Ubuntu/Debian)

love-pd-audio
=============

audio module for lua / love2d (win.dll/linux.so/mac?) using puredata for interactive audio manipulation/generation<br>

PureData files should make it possible to play midi files using sampled instruments, do realtime audio-generation and modification with configurable effect pipes like reverb etc, <br>
as well as all do advanced audio feedback like modifying music dynamically based on game happenings, and using the current point in the music to customize sound effects to fit in better.<br>

PureData is a tool to combine effects and generators in a visual way, usable by audio artists without any experience in text-based programming.<br>
The combinations are saved as *.pd files, which can be played in löve2D using this module.<br>
Signals can be sent and received to make the music interactive and/or tweak game elements based on the audio.<br>

Some examples from existing games that should be possible to realise using PureData:
* music speed and mood adapts to the speed of a player balancing/moving on some big round object
* time-limited power-up music becomes faster and more panicked as time runs out
* sound effects (gui:character-select/level-select, ingame jump-on-buttons) are adjusted to fit the currently playing notes in the music
* music instruments are modified when player enters/exits water, also reverb etc..
* bonus level with music being played/advanced as player collects a row of items
* enemy noises adjusted depending on distance to player
* music adjusted based on player hitpoints and current ingame mood/area (fight/peaceful/cave/plains/water/...)
* synching player steps or other game elements to music rythm or midi notes

2012-09 ghoulsblade@schattenkind.net

* http://love2d.org
* http://puredata.info
* https://github.com/libpd/libpd
* https://love2d.org/forums/viewtopic.php?f=5&t=10889  Love2D Forum Thread

see above for licensing details since code from there was used, they all use BSD/MIT style or similar, so even binary distribution should be ok, as long as the copyright notices are included.

API
----

	require("lovepdaudio")
	function libpdhook (event,...) print("libpdhook",event,...) end
	player = lovepdaudio.CreatePureDataPlayer(path_file,path_folder or ".",delay_msec or 100,num_buffers or 4)
	while (true) do lovepdaudio.PureDataPlayer_Update(player) end
	-- most libpd_* functions from https://github.com/libpd/libpd/wiki/libpd

EXAMPLES
--------

* test.lua for pure lua usage
* cd bin && lua test.lua 

* main.lua for love2d usage
* cd bin && love .     

* test.pd from puredata
* pdnes.pd from Filippo : http://www.fbpsound.com/

BUGS and ISSUES
---------------
* opening .pd files inside love2d packed zip/.love does not work yet (physfs)
* audible chops and cLuaAudioStream::resumePlayback printed when update is not called quickly enough
* luabindings for libpd incomplete: complex message, array access, some hooks
* several audio parameters like samplerate=44k/channels=2=stereo/16bits/maxbuffer=32/... are currently hardcoded
* audio problems if output samples by pd outside [-1,1], clipping has to be done in the pd file if neccessary
* no microphone input
* no midi output
* currently still some debug output on stdout for pinpointing problems
* linux 32bit and 64bit .so available, but only tested under ubuntu 11.04 64bit, different distro might need recompile
* win binaries requires 32bit love2d binary, luckily that works fine under 64bit windows
* win compile needs mingw, since libpd compile in msvc didn't work due to lack of C99 support, and i couldn't get lovepdaudio-msvc to cooperate with mingw-compiled-libpd
* linux users on 32 bit need to rename/link lovepdaudio.so.64 to lovepdaudio.so manually
* mac binaries not available yet, mac compile doesn't work yet, the Makefile is based on the one from libpd, and has mac stuff, but untested and still in need of adjusting.

NOTES
-----

So far this lua module doesn't even need löve2d and can be used with pure lua.

Interfacing directly into the love audio via sounddata in love 0.8.0 will not work properly due to lack of buffering/streaming api for runtime-generated audio-data.<br>
Instead the love2d code for openal audio source with streaming will be used as basis, and a new openal-instance will be opened by the module, independent from löve, to avoid needing a modified love2d binary.<br>

Even without using the PureData part it will probably make sense to expose an api for buffered/streamed realtime data that can be generated by lua. (TODO)<br>

Compile on win using http://www.mingw.org/ , browse to folder in mingw shell, run "make" (uses Makefile)

See libpd-makefile.diff for how to compile libpd statically under linux

Porting to webplayer might be possible on firefox and maybe also chrome using the raw audio api extension, but that's low prio for now, and won't work for standard html5 audio api in other browsers. (TODO) 
* https://wiki.mozilla.org/Audio_Data_API 
* http://chromium.googlecode.com/svn/trunk/samples/audio/index.html
