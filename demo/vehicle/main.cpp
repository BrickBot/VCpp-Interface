//main.cpp - main windows program for vehicle example
//(c) 11.2002 by Daniel Berger, MPI for Biological Cybernetics, T�bingen
//Don't forget to include vfw32.lib with the linked libraries!

#include <windows.h>
#include <stdio.h>
#include <vfw.h>
#include "resource.h"
#include "rcx21.h"
#include "videoin.h"
#include "vehicle.h"

char szAppName[]="vehicle";
HINSTANCE ghInstApp;
HINSTANCE ghPrevInstApp;
HWND ghWndMain = NULL;
int leaveprogram=0;
char msgstring[256]="";
int driver2use=0;
int port2use=0;

///////////////////////////  Screen Graphics Output Stuff  //////////////////

HBITMAP D_backbitmap;
static BYTE* Bits;
int D_width,D_height;
unsigned char *D_screenbuf;

void setupDIB(int width, int height, BOOL init){
  if (init){
    HDC dc=CreateCompatibleDC(NULL);
    HANDLE heap=GetProcessHeap();
    BITMAPINFO* format= (BITMAPINFO*)HeapAlloc(heap,0,sizeof(BITMAPINFOHEADER));
    BITMAPINFOHEADER* header= (BITMAPINFOHEADER*) format;
    header->biSize= sizeof(BITMAPINFOHEADER);
    header->biWidth= width;
    header->biHeight= -height;
    header->biPlanes= 1;
    header->biBitCount= 24;
    header->biCompression= BI_RGB;
    header->biSizeImage= 0;
    header->biXPelsPerMeter= 0;
    header->biYPelsPerMeter= 0;
    header->biClrUsed= 0;
    header->biClrImportant= 0;

    D_backbitmap=CreateDIBSection(dc,format,DIB_RGB_COLORS,(void**)&Bits,NULL,0);

    HeapFree(heap,0,format);
    DeleteDC(dc);

    D_width=width;
    D_height=height;
    D_screenbuf=Bits;

    for (int i=0; i<width*height*3; i++) Bits[i]=255;  //clear bitmap
    
  } else {
    DeleteObject(D_backbitmap);
  }
}

/////////////// Video Input Dialog Handling //////////////////

char text1[256];
char text2[256];

BOOL CALLBACK VisualInputProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam){
  WORD cmd;
  int i,k;
  
  switch(message){
  case WM_INITDIALOG:
    k=0;
    for (i=0; i<10; i++){
      capGetDriverDescription(i,text1,255,text2,255);
      if (text1[0]!=0){
        SendMessage(GetDlgItem(window,IDC_COMBO1),CB_ADDSTRING,0,(long)&text1);
        k++;
      }
    }
    if (driver2use>=k) driver2use=0;
    SendMessage(GetDlgItem(window,IDC_COMBO1),CB_SETCURSEL,driver2use,0);
    return(TRUE);

  case WM_CLOSE:
    EndDialog(window, FALSE);
    return(TRUE);

  case WM_COMMAND:
    cmd = LOWORD(wparam);
    switch(cmd){

    case IDOK:
      driver2use=SendMessage(GetDlgItem(window,IDC_COMBO1),CB_GETCURSEL,0,0);
      EndDialog(window, TRUE);
      return(TRUE);

    case IDCANCEL:
      EndDialog(window, FALSE);
      return(TRUE);

    default:
      return(FALSE);
    }
  }  
  return(FALSE);
}

int ShowVisualInputDialog(HINSTANCE WinInstance, HWND WinHandle){
  int r,olddriver2use;

  olddriver2use=driver2use;
  r=DialogBox(WinInstance, MAKEINTRESOURCE(IDD_VIDEOINPUT), WinHandle, (DLGPROC)VisualInputProc);

  if (olddriver2use!=driver2use){ //check if video source has changed
    //close old video input
    V_leavevideo();
    //open new video input
    if (V_initvideo(ghInstApp, ghPrevInstApp, driver2use, 320, 240)){
      V_startcapture();
      V_allowcopying=1;
    }
  }
  return(r);
}


/////////////// COM Port Dialog Handling //////////////////

BOOL CALLBACK ComPortDialogProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam){
  WORD cmd;
  int i,k;
  
  switch(message){
  case WM_INITDIALOG:
    sprintf(text1,"USB Tower 1");
    SendMessage(GetDlgItem(window,IDC_COMBO1),CB_ADDSTRING,0,(long)&text1);
    sprintf(text1,"USB Tower 2");
    SendMessage(GetDlgItem(window,IDC_COMBO1),CB_ADDSTRING,0,(long)&text1);
    k=2;
    for (i=0; i<9; i++){
      sprintf(text1,"COM%i",i+1);
      SendMessage(GetDlgItem(window,IDC_COMBO1),CB_ADDSTRING,0,(long)&text1);
      k++;
    }

    if (port2use>=k) port2use=1;
    SendMessage(GetDlgItem(window,IDC_COMBO1),CB_SETCURSEL,port2use,0);
    return(TRUE);

  case WM_CLOSE:
    EndDialog(window, FALSE);
    return(TRUE);

  case WM_COMMAND:
    cmd = LOWORD(wparam);
    switch(cmd){

    case IDOK:
      port2use=SendMessage(GetDlgItem(window,IDC_COMBO1),CB_GETCURSEL,0,0);
      EndDialog(window, TRUE);
      return(TRUE);

    case IDCANCEL:
      EndDialog(window, FALSE);
      return(TRUE);

    default:
      return(FALSE);
    }
  }  
  return(FALSE);
}

int ShowComPortDialog(HINSTANCE WinInstance, HWND WinHandle){

  int r=DialogBox(WinInstance, MAKEINTRESOURCE(IDD_COMPORT), WinHandle, (DLGPROC)ComPortDialogProc);

  //stop rcx interfacing
  if (RCX_stop!=-1){ //in case RCX communication is running
    RCX_stop=1;
    while (RCX_stop);  //wait until interface task has finished
  }
  //try to open tower communication on new port
  if (port2use<2){ //USB tower
    sprintf(text1,"\\\\.\\LEGOTOWER%i",port2use+1);
    if (!RCX_start(2,text1)){
      MessageBox (NULL, TEXT ("WARNING: Could not initialize USB RCX communication! Please check port settings and verify that the RCX is switched on."), szAppName, MB_ICONWARNING); 
    }
  } else { //serial tower (com port)
    sprintf(text1,"COM%i",port2use-1);
    if (!RCX_start(1,text1)){
      MessageBox (NULL, TEXT ("WARNING: Could not initialize serial RCX communication! Please check port settings and verify that the RCX is switched on."), szAppName, MB_ICONWARNING); 
    }
  }
  return(r);
}


///////////////////////// Main Windows Message Handler /////////////////////////

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
  HMENU hMenu;
  HDC hdc;
  PAINTSTRUCT ps;
  HDC memorydc;
  HBITMAP oldbitmap;

  switch (message){
  case WM_CREATE:
    return 0;

  case WM_COMMAND:
    hMenu = GetMenu (hwnd);
    switch (LOWORD (wParam)){  //process menu messages

    case ID_FILE_QUIT:
      leaveprogram=1;
      SendMessage (hwnd, WM_CLOSE, 0, 0);
      return 0;

    case ID_OPTIONS_SELECTVIDEOSOURCE:
      ShowVisualInputDialog(ghInstApp, ghWndMain);
      break;

    case ID_OPTIONS_SELECTTOWERCOMPORT:
      ShowComPortDialog(ghInstApp, ghWndMain);
      break;

    }
    break;

  case WM_DESTROY:
    leaveprogram=1;
    PostQuitMessage(0);
    return 0;

  case WM_PAINT:
    hdc = BeginPaint(hwnd, &ps);
    memorydc=CreateCompatibleDC(hdc);
    oldbitmap=(HBITMAP)SelectObject(memorydc,D_backbitmap);
    BitBlt(hdc,0,0,D_width,D_height,memorydc,0,0,SRCCOPY);

    //paint red area center cross
    MoveToEx(hdc,red_mx-5,red_my,NULL);
    LineTo(hdc,red_mx+5,red_my);
    MoveToEx(hdc,red_mx,red_my-5,NULL);
    LineTo(hdc,red_mx,red_my+5);

    SelectObject(memorydc,oldbitmap);
    DeleteDC(memorydc);
    EndPaint(hwnd, &ps);
    return(0);
  
  default:
    return DefWindowProc (hwnd, message, wParam, lParam);

  }
  return(0);
}

void messageCheck(){
//message translation polling (call this when convenient)
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow){
  HWND hwnd;
  MSG msg;
  WNDCLASS wndclass;

  ghInstApp=hInstance;
  ghPrevInstApp=hPrevInstance;
     
  //Define main window class
  wndclass.style         = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc   = WndProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = hInstance;
  wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
  wndclass.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
  wndclass.lpszClassName = szAppName;

  if (!RegisterClass (&wndclass)){
    MessageBox (NULL, TEXT ("ERROR: Windows refuses to register the windows class."), szAppName, MB_ICONERROR) ;
    return 0;
  }

  hwnd = CreateWindow (
    szAppName, 
    TEXT ("vehicle - Braitenberg vehicle example"),
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    330, 290,
    NULL, NULL,
    hInstance, NULL);
     
  ghWndMain=hwnd;

  setupDIB(320,240,TRUE);
  ShowWindow (hwnd, iCmdShow);
  UpdateWindow (hwnd);

  if (V_initvideo(hInstance, hPrevInstance, driver2use, 320, 240)){
    V_startcapture();
    V_allowcopying=1;
  } else {
    MessageBox (NULL, TEXT ("WARNING: Could not initialize video source."), szAppName, MB_ICONWARNING);
  }

  //start rcx interfacing; the tower is assumed to be connected to serial port 1 (COM1). Change if necessary.
  ShowComPortDialog(ghInstApp, ghWndMain);
/*  if (!RCX_start("COM1")){
    MessageBox (NULL, TEXT ("WARNING: Could not initialize RCX communication! Please check port settings and verify that the RCX is switched on."), szAppName, MB_ICONWARNING); 
  }*/

  RCX_motor_val[0]=0;
  RCX_motor_val[2]=0;

  RCX_motor_on[0]=1;
  RCX_motor_on[2]=1;

  while (!leaveprogram){  // main loop
    messageCheck();
    Sleep(50);
    if (V_newframeavailable) memcpy(D_screenbuf,V_inscreen,320*240*3);
    if (V_inscreen!=NULL) bullvehicle(V_inscreen);
    InvalidateRect(hwnd,0,0);
  }

  PostQuitMessage(0);
  while (GetMessage(&msg, NULL, 0, 0)){
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  RCX_motor_on[0]=0;
  RCX_motor_on[2]=0;
  Sleep(200);

  //stop rcx interfacing
  if (RCX_stop!=-1){ //in case RCX communication is running
    RCX_stop=1;
    while (RCX_stop);  //wait until interface task has finished
  }

  V_leavevideo();
  setupDIB(0,0,FALSE);

  return msg.wParam;
}

