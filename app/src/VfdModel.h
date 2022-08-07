#ifndef VfdModel_h
#define VfdModel_h

#include <Arduino.h>

struct ModbusParameterObjectModel
{
    char *name[16];
    int modbusAddress;
    int memoryValue;
}
