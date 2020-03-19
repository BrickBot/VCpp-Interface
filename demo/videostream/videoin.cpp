//videoin.cpp
//video input streaming using VideoForWindows
//V 1.0 (c) 11.2002 by Daniel Berger
//please remember to include vfw32.lib with the linked libraries!

#include <windows.h>
#include <vfw.h>
#include "main.h"

HWND ghWndCap;

CAPSTATUS      gCapStatus;
CAPDRIVERCAPS  gCapDriverCaps;
CAPTUREPARMS   gCapParms;
BITMAPINFO     videoformat;

FARPROC        fpErrorCallback;
FARPROC        fpStatusCallback;
FARPROC        fpVideoCallback;

int V_newframeavailable=0;
int V_is_capturing=0;
int V_allowcopying=0;

int vidwidth=0;   //local storage of the video image size to be used
int vidheight=0;

unsigned char *V_inscreen=NULL; //the video image will be provided in this variable later

////////////////////////////  Video Callback Procedures  /////////////////////////////

LRESULT FAR PASCAL ErrorCallbackProc(HWND hWnd, int nErrID, LPSTR lpErrorText){
  if (!ghWndMain) return FALSE;
  if (nErrID == 0) return TRUE;
  MessageBox(hWnd, lpErrorText, "videoin", MB_OK | MB_ICONEXCLAMATION);  // Show error message
  return (LRESULT) TRUE ;
}

LRESULT FAR PASCAL StatusCallbackProc(HWND hWnd, int nID, LPSTR lpStatusText){
  static int CurrentID;

  if (!ghWndMain) return FALSE;
  if (nID == IDS_CAP_END){  // the CAP_END message sometimes overwrites a useful statistics message.
    if ((CurrentID == IDS_CAP_STAT_VIDEOAUDIO)||(CurrentID == IDS_CAP_STAT_VIDEOONLY)){
      return(TRUE);
    }
  }
  CurrentID = nID;
  return (LRESULT) TRUE ;
}

LRESULT CALLBACK VideoStreamCallbackProc(HWND hWnd, LPVIDEOHDR lpVHdr){
//This is where the actual captured frame can be read
  if ((V_inscreen!=NULL) && (V_allowcopying)){
    for (int i=0; i<vidheight; i++){ //flip image upside down during copying
      memcpy(V_inscreen+i*vidwidth*3,(lpVHdr->lpData)+(vidheight-i-1)*vidwidth*3,vidwidth*3);
    } 
    //memcpy(V_inscreen,(lpVHdr->lpData),vidwidth*vidheight*3);
  }
  V_newframeavailable=1;
  return (LRESULT) TRUE;
}

void V_SetLive(BOOL bLive){
  capPreview(ghWndCap, bLive);
}

void V_SetOverlay(BOOL bOverlay){
  if (!gCapDriverCaps.fHasOverlay) return;
  capOverlay(ghWndCap, bOverlay);
}

int V_startcapture(){
  videoformat.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  videoformat.bmiHeader.biWidth=vidwidth;
  videoformat.bmiHeader.biHeight=vidheight;
  videoformat.bmiHeader.biPlanes=1;
  videoformat.bmiHeader.biBitCount=24;
  videoformat.bmiHeader.biCompression=BI_RGB;
  videoformat.bmiHeader.biSizeImage=0;
  videoformat.bmiHeader.biXPelsPerMeter=0;
  videoformat.bmiHeader.biYPelsPerMeter=0;
  videoformat.bmiHeader.biClrUsed=0;
  videoformat.bmiHeader.biClrImportant=0;
  if (!capSetVideoFormat(ghWndCap,&videoformat,sizeof(BITMAPINFO))) return(0);

  gCapParms.dwRequestMicroSecPerFrame=40000; //25 Hz //(20000 = 50 Hz Sampling)
  gCapParms.fMakeUserHitOKToCapture=FALSE;   // dialog box to start?
  gCapParms.wPercentDropForError=70;         // stop recording if 70% of frames are lost
  gCapParms.fYield=TRUE;                    // use extra thread for capturing?
  gCapParms.dwIndexSize=0;                   // (only needed when writing to files)
  gCapParms.wChunkGranularity=0;               // (only needed when writing to files)
  gCapParms.fUsingDOSMemory=0;                 //unused in win32.
  gCapParms.wNumVideoRequested=50; //200          //maximum number of video buffers to be allocated
  gCapParms.fCaptureAudio=FALSE;             //capture audio as well?
  gCapParms.wNumAudioRequested=10;           //maximum number of audio buffers to be allocated
  gCapParms.vKeyAbort=VK_ESCAPE | 0x8000;    //key to end capturing (STRG-ESCAPE) - causes crash when used!
  gCapParms.fAbortLeftMouse=FALSE;           //stop capturing when left mouse button pressed?
  gCapParms.fAbortRightMouse=FALSE;          //stop capturing when right mouse button pressed?
  gCapParms.fLimitEnabled=FALSE;             //stop after time limit?
  gCapParms.wTimeLimit=0;                      //according time limit
  gCapParms.fMCIControl=FALSE;               //is this an MCI Device?
  gCapParms.fStepMCIDevice=FALSE;  //ignored
  gCapParms.dwMCIStartTime=0;    //ignored
  gCapParms.dwMCIStopTime=0;     //ignored
  gCapParms.fStepCaptureAt2x=FALSE; 
  gCapParms.wStepCaptureAverageFrames=5;
  gCapParms.dwAudioBufferSize=0; 
  gCapParms.fDisableWriteCache=FALSE; //unused in win32 programs
  gCapParms.AVStreamMaster=AVSTREAMMASTER_NONE; 
  if (!capCaptureSetSetup(ghWndCap, &gCapParms, sizeof(CAPTUREPARMS))) return(0);
  if (!capCaptureSequenceNoFile(ghWndCap)) return(0);
  V_is_capturing=1;
  return(1);
}

int V_initvideo(HINSTANCE hInstance, HINSTANCE hPrevInstance, int capdrivernr, int width, int height){
  //returns 0 if failed, 1 if success
  vidwidth=width; vidheight=height;

  ghWndCap = capCreateCaptureWindow("myCaptureWindow",WS_CHILD,0, 0, vidwidth, vidheight, ghWndMain,1);
  if (ghWndCap == NULL) return(0);

  // Register the status and error callbacks before driver connects
  // so we can get feedback about the connection process
  fpErrorCallback = MakeProcInstance((FARPROC)ErrorCallbackProc, ghInstApp);
  capSetCallbackOnError(ghWndCap, fpErrorCallback);

  fpStatusCallback = MakeProcInstance((FARPROC)StatusCallbackProc, ghInstApp);
  capSetCallbackOnStatus(ghWndCap, fpStatusCallback);
  
  //Video Stream Callback
  fpVideoCallback = MakeProcInstance((FARPROC)VideoStreamCallbackProc, ghInstApp);
  capSetCallbackOnVideoStream(ghWndCap, fpVideoCallback);

  // Get the default setup for video capture from the AVICap window
  capCaptureGetSetup(ghWndCap, &gCapParms, sizeof(CAPTUREPARMS));

  // Try connecting to the capture driver
  if (!capDriverConnect(ghWndCap, capdrivernr)) return(0);

  // Get the capabilities of the capture driver
  if (!capDriverGetCaps(ghWndCap, &gCapDriverCaps, sizeof(CAPDRIVERCAPS))) return(0);

  // Get the settings for the capture window
  if (!capGetStatus(ghWndCap, &gCapStatus , sizeof(gCapStatus))) return(0);

  // set live/overlay to default
  V_SetOverlay(FALSE);
  V_SetLive(FALSE);

  V_inscreen=new unsigned char[vidwidth*vidheight*3];

  return(1);
}

void V_leavevideo(){
  if (V_is_capturing){
    capCaptureAbort(ghWndCap);
    V_is_capturing=0;
  }

  // Disable and free all the callbacks 
  capSetCallbackOnVideoStream(ghWndCap,NULL);
  if (fpVideoCallback){
    FreeProcInstance(fpVideoCallback);
		fpVideoCallback=NULL;
	}

  capSetCallbackOnStatus(ghWndCap, NULL);
	if (fpStatusCallback){
    FreeProcInstance(fpStatusCallback);
		fpStatusCallback=NULL;
	}

  capSetCallbackOnError(ghWndCap, NULL);
	if (fpErrorCallback){
    FreeProcInstance(fpErrorCallback);
		fpErrorCallback=NULL;
	}

  // Disconnect the current capture driver
  capDriverDisconnect (ghWndCap);

  V_allowcopying=0;
  delete [] V_inscreen;
  V_inscreen=NULL;

  // Destroy this window
  DestroyWindow(ghWndCap);
}

int V_showvideosourcedialog(){
  //returns -1 if dialog unsupported, 0 if dialog failed, 1 if successful
  if (gCapDriverCaps.fHasDlgVideoSource==1) return(capDlgVideoSource(ghWndCap));
  return(-1);
}

int V_showvideoformatdialog(){
  //returns -1 if dialog unsupported, 0 if dialog failed, 1 if successful
  if (gCapDriverCaps.fHasDlgVideoFormat==1) return(capDlgVideoFormat(ghWndCap));
  return(-1);
}

int V_showvideodisplaydialog(){
  //returns -1 if dialog unsupported, 0 if dialog failed, 1 if successful
  if (gCapDriverCaps.fHasDlgVideoDisplay==1) return(capDlgVideoDisplay(ghWndCap));
  return(-1);
}