#ifndef UTIL_H
#define UTIL_H

#define htons(x) ( (((x)<<8)&0xFF00) | (((x)>>8)&0xFF) )
#define ntohs(x) htons(x)

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )
#define ntohl(x) htonl(x)

#if __DEBUG_WIZNET__        
#define WIZNET_DEBUG(...) Serial.print(__VA_ARGS__)
#define WIZNET_DEBUGLN(...) Serial.println(__VA_ARGS__)
#else
#define WIZNET_DEBUG(...)
#define WIZNET_DEBUGLN(...)
#endif


#endif
