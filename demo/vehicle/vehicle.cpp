//vehicle.cpp
//(c) 11.2002 by Daniel Berger, MPI for Biological Cybernetics, Tübingen

#include "rcx21.h"

int red_mx,red_my,red_area; //global variables for red area in visual field

void findredarea(unsigned char *rawimage){
  //calculates mean point and area of red things in the visual field
  //stores results in global variables red_mx, red_my and red_area
  //assumes that rawimage is 320x240 rgb
  int x,y;
  int p=0;

  red_area=0; red_mx=0; red_my=0;
  for (y=0; y<240; y++){
    for (x=0; x<320; x++){
      if ((rawimage[p+2]>(rawimage[p]+rawimage[p+1]))&&((rawimage[p]>30)&&(rawimage[p+1]>30))){ //this is a red pixel
        red_mx=red_mx+x;
        red_my=red_my+y;
        red_area++;
      }
      p=p+3;
    }
  }
  if (red_area==0){
    red_mx=-1;
    red_my=-1;
  } else {
    red_mx=red_mx/red_area;
    red_my=red_my/red_area;
  }
}

void bullvehicle(unsigned char *rawimage){
  //a 'bull' vehicle that is attracted by red things
  double lval,rval;

  findredarea(rawimage);

  //rotate robot so that red area goes towards the middle of the image
  lval=((double)red_mx-160.0)/40.0;
  rval=-((double)red_mx-160.0)/40.0;

  //go forward according to size of red area
  lval=lval+((double)red_area/3000.0);
  rval=rval+((double)red_area/3000.0);

  //value clipping for motor output
  if (lval<-8.0) lval=-8.0; if (lval>8.0) lval=8.0;
  if (rval<-8.0) rval=-8.0; if (rval>8.0) rval=8.0;

  //output values to motors
  RCX_motor_val[0]=(int)lval;
  RCX_motor_val[2]=(int)rval;
}