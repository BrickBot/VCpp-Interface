//videoin.h
//header file for videoin.cpp
//V 1.0 (c) 11.2002 by Daniel Berger

int V_initvideo(HINSTANCE hInstance, HINSTANCE hPrevInstance, int capdrivernr, int width, int height);
void V_leavevideo();
int V_showvideosourcedialog();
int V_showvideoformatdialog();
int V_showvideodisplaydialog();

int V_startcapture();
void V_SetLive(BOOL bLive);
void V_SetOverlay(BOOL bOverlay);

extern int V_newframeavailable;
extern int V_is_capturing;
extern int V_allowcopying;
extern unsigned char *V_inscreen;