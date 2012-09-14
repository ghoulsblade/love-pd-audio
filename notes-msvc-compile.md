Big thanks to Boolsheet from Löve2D.org IRC for this info on how to compile libpd and lovepdaudio using the msvc commandline tools :



I downloaded the latest libpd from their github repository.<br>
Then I had to add some braces in z_libpd.c to make it C89 compatible.

libpd_wrapper/z_libpd.c:153
Change the MEMCPY define to this.

	#define MEMCPY(_x, _y) \
	  GETARRAY \
	  if (n < 0 || offset < 0 || offset + n > garray_npoints(garray)) return -2; \
	  {t_word *vec = ((t_word *) garray_vec(garray)) + offset; \
	  int i; \
	  for (i = 0; i < n; i++) _x = _y;}

libpd_wrapper/z_libpd.c:348
Change it the libpd_getdollarzero function to this.

	int libpd_getdollarzero(void *x) {
	  pd_pushsym((t_pd *)x);
	  {int dzero = canvas_getdollarzero();
	  pd_popsym((t_pd *)x);
	  return dzero;}
	}

Visual Studio and the Window SDK provide scripts to setup build environments for the command prompt.<br>
I use the one from the SDK (C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.Cmd).<br>
After executing that, you can change the target with "setenv /x86" and "setenv /x64".<br>

Building libpd. Assuming working directory is the root of libpd repository.


	cl /nologo /c /O2 /D PD_INTERNAL /D WIN32 /I <path to pthread.h> pure-data\src\*.c
	cl /nologo /c /O2 /D PD_INTERNAL /D WIN32 /I pure-data\src libpd_wrapper\z_libpd.c
	cl /nologo /c /O2 /D PD_INTERNAL /D WIN32 /I pure-data\src libpd_wrapper\s_libpdmidi.c
	cl /nologo /c /O2 /D PD_INTERNAL /D WIN32 /I pure-data\src libpd_wrapper\x_libpdreceive.c

	link /nologo /DLL /OUT:libpd.dll *.obj ws2_32.lib advapi32.lib <your pthread.lib>


Building love-pd-audio. Assuming working directory is the root of love-pd-audio repository.

	cl /nologo /c /O2 /EHsc /I <path to pure-data\src> /I <path to libpd_wrapper> /I <path to OpenAL include dir> /I <path to lua.h> lovepdaudio.cpp
	link /nologo /DLL /OUT:lovepdaudio.dll lovepdaudio.obj <the libpd.lib generated above> <your OpenAL.lib> <your lua51.lib>


How this looked for me.

	cl /nologo /c /O2 /D PD_INTERNAL /D WIN32 /I pthread pure-data\src\*.c 
	cl /nologo /c /O2 /D PD_INTERNAL /D WIN32 /I pure-data\src libpd_wrapper\z_libpd.c
	cl /nologo /c /O2 /D PD_INTERNAL /D WIN32 /I pure-data\src libpd_wrapper\s_libpdmidi.c
	cl /nologo /c /O2 /D PD_INTERNAL /D WIN32 /I pure-data\src libpd_wrapper\x_libpdreceive.c
	link /nologo /DLL /OUT:libpd.dll *.obj ws2_32.lib advapi32.lib pthread\pthreadVSE2.lib


	cl /nologo /c /O2 /EHsc /I ..\libpd\pure-data\src /I ..\libpd\libpd_wrapper /I ..\openal-soft-1.14\include /I ..\lua-5.1.5\src lovepdaudio.cpp
	link /nologo /DLL /OUT:lovepdaudio.dll lovepdaudio.obj ..\libpd\libpd.lib ..\openal-soft-1.14\OpenAL.lib ..\lua-5.1.5\lua51.lib


Bear in mind that this is the very minimum to get it working.<br>
There are other options that might be relevant, especially for producing code that is easier to debug.<br>
See "cl.exe /?", "link.exe /?", and the msdn website.<br>

