#include "eventhandler.h"


#include "joyconthread.h" // tk move mappings to a better place

// key code mappings
//https://stackoverflow.com/questions/1918841/how-to-convert-ascii-character-to-cgkeycode/14529841#14529841][1]

// xlib mouse control
//https://forum.openframeworks.cc/t/how-to-set-mouse-position/1438/6

EventHandler::EventHandler()
{
}

void EventHandler::handleMouseMove(double dx, double dy){
    Display *display = XOpenDisplay(0);
    XWarpPointer(display, None, None, 0, 0, 0, 0, delta_x, delta_y);
    XCloseDisplay(display);
}

void EventHandler::handleButtonPress(int button_mask){

}

void EventHandler::handleButtonRelease(int button_mask){

}
