/*
Entry point to the wiznet drivers
*/
#ifndef	WIZNET_H_INCLUDED
#define	WIZNET_H_INCLUDED

#include "_wiznet.h"

//#define W5100_ETHERNET_SHIELD // Arduino Ethenret Shield and Compatibles ...
//#define W5200_ETHERNET_SHIELD // WIZ820io, W5200 Ethernet Shield 
//#define W5500_ETHERNET_SHIELD   // WIZ550io, ioShield series of WIZnet

#if defined(USE_W5500)
//#define USE_BURNED_MACADDRESS // Use assigned MAC address of WIZ550io
#include "w5500.h"
#endif

#if defined(USE_W5200)
#include "w5200.h"
#endif

#if defined(USE_W5100)
#include "w5100.h"
#endif

#endif // WIZNET_H_INCLUDED