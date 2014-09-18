/*
Entry point to the wiznet drivers
*/
#ifndef	WIZNET_H_INCLUDED
#define	WIZNET_H_INCLUDED

// FIXME: If I ever want this to be included in mainline Arduino, need a better way to select which chip to use

#include "_wiznet.h"

//#define USE_W5100 // Arduino Ethenret Shield and Compatibles ...
//#define USE_W5200 // WIZ820io, W5200 Ethernet Shield 
//#define USE_W5500 // WIZ550io, ioShield series of WIZnet

#if defined(USE_W5500)
//#define USE_BURNED_MACADDRESS // Use assigned MAC address of WIZ550io
#include "w5500.h"
#elif defined(USE_W5200)
#include "w5200.h"
#elif defined(USE_W5100)
#include "w5100.h"
#else
#error "Did not define Wiznet chip to use."
#endif

#endif // WIZNET_H_INCLUDED