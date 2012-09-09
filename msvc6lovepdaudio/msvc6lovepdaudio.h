
// Der folgende ifdef-Block zeigt die Standardlösung zur Erstellung von Makros, die das Exportieren 
// aus einer DLL vereinfachen. Alle Dateien in dieser DLL wurden mit dem in der Befehlszeile definierten
// Symbol MSVC6LOVEPDAUDIO_EXPORTS kompiliert. Dieses Symbol sollte für kein Projekt definiert werden, das
// diese DLL verwendet. Auf diese Weise betrachtet jedes andere Projekt, dessen Quellcodedateien diese Datei 
// einbeziehen, MSVC6LOVEPDAUDIO_API-Funktionen als aus einer DLL importiert, während diese DLL mit diesem 
// Makro definierte Symbole als exportiert betrachtet.
#ifdef MSVC6LOVEPDAUDIO_EXPORTS
#define MSVC6LOVEPDAUDIO_API __declspec(dllexport)
#else
#define MSVC6LOVEPDAUDIO_API __declspec(dllimport)
#endif

// Diese Klasse ist aus msvc6lovepdaudio.dll exportiert
class MSVC6LOVEPDAUDIO_API CMsvc6lovepdaudio {
public:
	CMsvc6lovepdaudio(void);
	// ZU ERLEDIGEN: Fügen Sie hier Ihre Methoden hinzu.
};

extern MSVC6LOVEPDAUDIO_API int nMsvc6lovepdaudio;

MSVC6LOVEPDAUDIO_API int fnMsvc6lovepdaudio(void);

