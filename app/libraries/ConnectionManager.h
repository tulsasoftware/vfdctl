/*
  ConnectionManager.h - Library for flashing ConnectionManager code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/
#ifndef ConnectionManager_h
#define ConnectionManager_h

#include "Arduino.h"

class ConnectionManager
{
  public:
    ConnectionManager(int pin);
    void dot();
    void dash();
  private:
    int _pin;
};

#endif