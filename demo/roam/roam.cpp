//roam.cpp
//a small demonstration program for the rcx21.cpp library
//(c) 15.4.2003 by Daniel Berger (daniel.berger@tuebingen.mpg.de)

/* Description:
The program assumes that the two motors of the vehicle are connected to motor outputs
A and C, and the front touch sensor is connected to sensor input 1.
All interfacing is done via the RCX_* variables.
The robot is going forward until an obstacle is hit, in the case of which it turns
and continues forward.
This is a windows console program which uses the conio.h console interface of Visual C++
for easy keyboard access and text output to the console window.
Simply put rcx2.cpp, rcx2.h and roam.cpp in a windows console project in Visual C++,
set the serial port used or USB tower number below (in the respective RCX_start command), 
compile, run and enjoy. :)
*/

#include <windows.h>
#include <conio.h>
#include "rcx21.h"

int main(int argc, char* argv[]){
  char ch;

  _cprintf("Please switch on the robot and place it in front of the tower. Then press 1 for serial port, 2 for usb port: ");
  ch=_getch();
  if ((ch!='1') && (ch!='2')) return(0);

  //start interfacing; the tower is assumed to be connected to serial port 1 (COM1)
  //or any USB port for USB (\\.\LEGOTOWER1). Change if necessary.
  if (ch=='1'){ //attempt to open tower at serial port
    if (!RCX_start(1,"COM1")){
      if (RCX_error==1){
        _cprintf("\nERROR: could not initialize serial port! Please check port settings!\n");
      } else {
        _cprintf("\nERROR: tower-robot communication failed! Make sure that the tower is connected, the robot is switched on and in reach and that you use the right firmware!\n");
      }
      return 0;
    }
  } else { //attempt to open USB tower
    if (!RCX_start(2,"\\\\.\\LEGOTOWER1")){
      if (RCX_error==1){
        _cprintf("\nERROR: could not initialize USB tower! Please check port settings!\n");
      } else {
        _cprintf("\nERROR: robot communication failed! Make sure that the robot is switched on and in reach and that you use the right firmware!\n");
      }
      return 0;
    }
  }

  //set motor speeds
  RCX_motor_val[0]=8;
  RCX_motor_val[2]=8;
  Sleep(100); //wait 100 milliseconds

  //switch on motors A,C
  RCX_motor_on[0]=1;
  RCX_motor_on[2]=1;
  //switch on sensor 1 (touch sensor)
  RCX_sensor_on[1]=1;

  _cprintf("Please press a key to stop program ...\n");

  while (!_kbhit()){
    if (RCX_sensor_val[1]<500){ //detected an obstacle?
      RCX_sound=1; //bip!
      _cprintf("| %i obstacle, turning|",RCX_sensor_val[1]);
      RCX_motor_val[0]=-8; //turn
      RCX_motor_val[2]=8;
      Sleep(200); //wait 200 ms
    }
    _cprintf(".");
    //move forward
    RCX_motor_val[0]=8;
    RCX_motor_val[2]=8;
    Sleep(100); //wait 100 ms
  }

  //swich off sensor 1 and motors A and C
  RCX_sensor_on[1]=0;
  RCX_motor_on[0]=0;
  RCX_motor_on[2]=0;
  Sleep(200); //wait 200 ms

  //stop interfacing
  RCX_stop=1;
  while (RCX_stop);  //wait until interface task has finished
	return 0;
}