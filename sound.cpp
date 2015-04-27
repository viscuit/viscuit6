#include <qdict.h>
#ifdef Q_OS_WIN32

#define WIN32_LEAN_AND_MEAN
//#define INITGUID

#include <dmusici.h> // need the arrow thingies, but they don't display in html
#include <windows.h>
#include <dmusicc.h>
#include <dmusicf.h>
#include <dmksctrl.h>

#include <windowsx.h>
//#include <fstream.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ks.h>

#if defined( _WIN32 ) && !defined( _NO_COM)
DEFINE_GUID( GUID_NULL , 0,0,0, 0,0,0,0,0,0,0,0 );
#endif


QDict<IDirectMusicSegment8> *gPtrDict = 0;

#define DXDELETE(p) if(p != NULL) p->Release(); // a SAFEDELETE replacement
// if arrays are deleted, be sure to use delete [] array; first.


// DX globals
IDirectMusicLoader8*		g_pLoader       = NULL;
IDirectMusicPerformance8*	g_pPerformance  = NULL;
IDirectMusicPort8*			g_pOutPort= NULL;
IDirectMusicBuffer8*		g_pDMBuffer=NULL;
IDirectMusic*				g_pDMusic=NULL;
IReferenceClock				*g_pLClock=NULL;
IDirectMusicCollection			*pDLSCollection=NULL;
IDirectMusicDownloadedInstrument *pDlInst=NULL;

void soundInit() {
	gPtrDict = new QDict<IDirectMusicSegment8>();

	CoInitialize(NULL);

   
    CoCreateInstance(CLSID_DirectMusicLoader, NULL, 
                     CLSCTX_INPROC, IID_IDirectMusicLoader8,
                     (void**)&g_pLoader);

    CoCreateInstance(CLSID_DirectMusicPerformance, NULL,
                     CLSCTX_INPROC, IID_IDirectMusicPerformance8,
                     (void**)&g_pPerformance );


	g_pPerformance->InitAudio( 
        &g_pDMusic,                  // IDirectMusic interface not needed.
        NULL,                  // IDirectSound interface not needed.
        0, //hwnd,                  // Window handle.
        DMUS_APATH_SHARED_STEREOPLUSREVERB,  // Default audiopath type.
        64,                    // Number of performance channels.
        DMUS_AUDIOF_ALL,       // Features on synthesizer.
        NULL                   // Audio parameters; use defaults.
    );
 
    g_pLoader->SetSearchDirectory( 
        GUID_DirectMusicAllTypes,   // Types of files sought.
          /*L"C:\\Program Files\\KaZaA\\My Shared Folder"*/ NULL, //Null for default
                                                                  // Where to look. was wstrSearchPath
        FALSE                       // Don't clear object data.
    );
#ifdef MIDI
	{
	DMUS_PORTPARAMS dmpparam;
	DMUS_BUFFERDESC desc;
	GUID guidPort;
	HRESULT hr;

	ZeroMemory( &dmpparam , sizeof( DMUS_PORTPARAMS ) );
	dmpparam.dwSize = sizeof( DMUS_PORTPARAMS );

	hr = g_pDMusic->GetDefaultPort( &guidPort );

	hr = g_pDMusic->CreatePort( guidPort , &dmpparam , &g_pOutPort , NULL ); 

	hr = g_pOutPort->Activate( TRUE );

	desc.dwSize		= sizeof( DMUS_BUFFERDESC );
	desc.guidBufferFormat = GUID_NULL;
	desc.cbBuffer	= 32;	// 28byteHeader + 4byteMIDIArea
	desc.dwFlags	= 0;

	hr = g_pDMusic->CreateMusicBuffer( &desc , &g_pDMBuffer , NULL ); 

	hr = g_pOutPort->GetLatencyClock( &g_pLClock ); 
}
	{
			DMUS_OBJECTDESC desc;
	HRESULT hr;

	if( pDLSCollection != NULL )
	{
		pDLSCollection->Release();
		pDLSCollection = NULL;
	}

	ZeroMemory( &desc , sizeof( DMUS_OBJECTDESC ) );
	desc.dwSize		= sizeof( DMUS_OBJECTDESC );
	desc.guidClass	= CLSID_DirectMusicCollection;

		desc.guidObject		= GUID_DefaultGMCollection;
		desc.dwValidData	= DMUS_OBJ_CLASS | DMUS_OBJ_OBJECT;

	hr = g_pLoader->GetObject( &desc , IID_IDirectMusicCollection , ( void ** )&pDLSCollection );

	}
#endif
}


// Game functions
	
void soundFinal(void)
{
    CoUninitialize();	
	g_pPerformance->Stop(
        NULL,   // Stop all segments.
        NULL,   // Stop all segment states.
        0,      // Do it immediately.
        0       // Flags.
    );
    g_pPerformance->CloseDown();
 
    DXDELETE(g_pLoader);
    DXDELETE(g_pPerformance);
   QDictIterator<IDirectMusicSegment8> it( *gPtrDict );
    for( ; it.current(); ++it )
		DXDELETE(it.current())
	delete gPtrDict;
}


int dmc_OutShortMsg( unsigned long msg , int msec)
{
	REFERENCE_TIME rt;
	HRESULT hr;

	hr = g_pLClock->GetTime( &rt );
	g_pDMBuffer->SetStartTime(rt+msec);
	hr = g_pDMBuffer->PackStructured( rt, 1 , msg ); 
	hr = g_pOutPort->PlayBuffer( g_pDMBuffer );
	hr = g_pDMBuffer->Flush();

	return 0;
}

int  soundPlay(QString fname) {
#ifdef MIDI
	if (fname.right(4)==".mid") {
		 {

		IDirectMusicInstrument *pDMInst;
		HRESULT hr;

		if( pDlInst != NULL )
		{
			g_pOutPort->UnloadInstrument( pDlInst );
			pDlInst->Release();
			pDlInst = NULL;
		}	

		hr = pDLSCollection->GetInstrument( 0x55 , &pDMInst );
		if( FAILED( hr ) )	return 1;

		hr = g_pOutPort->DownloadInstrument( pDMInst , &pDlInst , NULL , 0 );
		if( FAILED( hr ) )	return 2;

		pDMInst->Release();

	
		}
		dmc_OutShortMsg(0x000055c0,0);
		dmc_OutShortMsg(0x007f3090,0);
		Sleep(1000);
		dmc_OutShortMsg(0x007f3080,1000000);
		return 0;
	}
#endif
	IDirectMusicSegment8 *segment;

	segment = gPtrDict->find(fname);
	if (!segment) {
	    if (FAILED(g_pLoader->LoadObjectFromFile(
		    CLSID_DirectMusicSegment,   // Class identifier.
			IID_IDirectMusicSegment8,   // ID of desired interface.
			(unsigned short *)fname.ucs2(),               // Filename.
			(LPVOID*) &segment       // Pointer that receives interface.
		))){
			qDebug("file not found %s", fname.ascii());
			return 0;
		}
//		qDebug("file read %s", fname.ascii());
		gPtrDict->insert(fname, segment);
	} else {
//		qDebug("file found %s", fname.ascii());	
	}
    segment->Download( g_pPerformance );
 
	
	g_pPerformance->PlaySegmentEx(
        segment,  // Segment to play.
        NULL,        // Used for songs; not implemented.
        NULL,        // For transitions. 
        DMUS_SEGF_SECONDARY,           // Flags.
        0,           // Start time; 0 is immediate.
        NULL,        // Pointer that receives segment state.
        NULL,        // Object to stop.
        NULL         // Audiopath, if not default.
    );
	return 1;
}

void soundSetup(QString fname, QString realFName) {
  IDirectMusicSegment8 *segment;
  if (FAILED(g_pLoader->LoadObjectFromFile(
					   CLSID_DirectMusicSegment,   // Class identifier.
					   IID_IDirectMusicSegment8,   // ID of desired interface.
					   (unsigned short *)realFName.ucs2(),               // Filename.
					   (LPVOID*) &segment       // Pointer that receives interface.
					   ))){
    qDebug("file not found %s", realFName.ascii());
    return;
  }
  gPtrDict->insert(fname, segment);
}


#else
int soundPlay(QString) { return 0; }
void soundInit() { }
void soundFinal() { }
void soundSetup(QString fname, QString realFName) { }
#endif
