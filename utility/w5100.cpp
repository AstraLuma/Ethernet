/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>

#if defined(__arm__)
#include "SPIFIFO.h"
#ifdef  HAS_SPIFIFO
#define USE_SPIFIFO
#endif
#endif

#include "Arduino.h"

#include "w5100.h"

#if defined(W5100_ETHERNET_SHIELD)

// W5100 controller instance
uint16_t W5100Class::SBASE[MAX_SOCK_NUM];
uint16_t W5100Class::RBASE[MAX_SOCK_NUM];
uint16_t W5100Class::CH_BASE;
uint16_t W5100Class::SSIZE;
uint16_t W5100Class::SMASK;
uint8_t  W5100Class::chip;
W5100Class W5100;

#define TX_RX_MAX_BUF_SIZE 2048
#define TX_BUF 0x1100
#define RX_BUF (TX_BUF + TX_RX_MAX_BUF_SIZE)

#define TXBUF_BASE 0x4000
#define RXBUF_BASE 0x6000

void W5100Class::init(void)
{
  uint8_t i;
  delay(300);
  //Serial.println("w5100 init");

#ifdef USE_SPIFIFO
  SPI.begin();
  SPIFIFO.begin(W5200_SS_PIN, SPI_CLOCK_12MHz);  // W5100 is 14 MHz max
#else
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  initSS();
#endif
  
  writeMR(1<<RST);
  writeTMSR(0x55);
  writeRMSR(0x55);

  for (int i=0; i<MAX_SOCK_NUM; i++) {
    SBASE[i] = TXBUF_BASE + SSIZE * i;
    RBASE[i] = RXBUF_BASE + RSIZE * i;
  }
  SPI.endTransaction();
  return 1; // successful init
}

void W5100Class::reset(void)
{
  uint16_t count=0;

  //Serial.println("W5100 reset");
  writeMR(1<<RST);
  while (++count < 20) {
    uint8_t mr = readMR();
    //Serial.print("mr=");
    //Serial.println(mr, HEX);
    if (mr == 0) break;
    delay(1);
  }
}

uint16_t W5100Class::getTXFreeSize(SOCKET s)
{
  uint16_t val=0, val1=0;
  do {
    val1 = readSnTX_FSR(s);
    if (val1 != 0)
      val = readSnTX_FSR(s);
  } 
  while (val != val1);
  return val;
}

uint16_t W5100Class::getRXReceivedSize(SOCKET s)
{
  uint16_t val=0,val1=0;
  do {
    val1 = readSnRX_RSR(s);
    if (val1 != 0)
      val = readSnRX_RSR(s);
  } 
  while (val != val1);
  return val;
}


void W5100Class::send_data_processing(SOCKET s, const uint8_t *data, uint16_t len)
{
  // This is same as having no offset in a call to send_data_processing_offset
  send_data_processing_offset(s, 0, data, len);
}

void W5100Class::send_data_processing_offset(SOCKET s, uint16_t data_offset, const uint8_t *data, uint16_t len)
{
  uint16_t ptr = readSnTX_WR(s);
  ptr += data_offset;
  uint16_t offset = ptr & SMASK;
  uint16_t dstAddr = offset + SBASE[s];

  if (offset + len > SSIZE) 
  {
    // Wrap around circular buffer
    uint16_t size = SSIZE - offset;
    write(dstAddr, data, size);
    write(SBASE[s], data + size, len - size);
  } 
  else {
    write(dstAddr, data, len);
  }

  ptr += len;
  writeSnTX_WR(s, ptr);
}


void W5100Class::recv_data_processing(SOCKET s, uint8_t *data, uint16_t len, uint8_t peek)
{
  uint16_t ptr;
  ptr = readSnRX_RD(s);
  read_data(s, ptr, data, len);
  if (!peek)
  {
    ptr += len;
    writeSnRX_RD(s, ptr);
  }
}

void W5100Class::read_data(SOCKET s, uint16_t src, volatile uint8_t *dst, uint16_t len)
{
  uint16_t size;
  uint16_t src_mask;
  uint16_t src_ptr;

  src_mask = (uint16_t)src & RMASK;
  src_ptr = RBASE[s] + src_mask;

  if( (src_mask + len) > RSIZE ) 
  {
    size = RSIZE - src_mask;
    read(src_ptr, (uint8_t *)dst, size);
    dst += size;
    read(RBASE[s], (uint8_t *) dst, len - size);
  } 
  else
    read(src_ptr, (uint8_t *) dst, len);
}


#ifdef USE_SPIFIFO
uint16_t W5100Class::write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
  uint32_t i;

  for (i=0; i<len; i++) {
    SPIFIFO.write16(0xF000 | (addr >> 8), SPI_CONTINUE);
    SPIFIFO.write16((addr << 8) | buf[i]);
    addr++;
    SPIFIFO.read();
    SPIFIFO.read();
  }
  return len;
}
#else
uint8_t W5100Class::write(uint16_t _addr, uint8_t _data)
{
  setSS();  
  SPI.transfer(0xF0);
  SPI.transfer(_addr >> 8);
  SPI.transfer(_addr & 0xFF);
  SPI.transfer(_data);
  resetSS();
  return 1;
}
#endif

uint16_t W5100Class::write(uint16_t _addr, const uint8_t *_buf, uint16_t _len)
{
  for (uint16_t i=0; i<_len; i++)
  {
    setSS();    
    SPI.transfer(0xF0);
    SPI.transfer(_addr >> 8);
    SPI.transfer(_addr & 0xFF);
    _addr++;
    SPI.transfer(_buf[i]);
    resetSS();
  }
  return _len;
}

#endif

#ifdef USE_SPIFIFO
uint16_t W5100Class::read(uint16_t addr, uint8_t *buf, uint16_t len)
{
  uint32_t i;

  for (i=0; i<len; i++) {
    #if 1
    SPIFIFO.write(0x0F, SPI_CONTINUE);
    SPIFIFO.write16(addr, SPI_CONTINUE);
    addr++;
    SPIFIFO.read();
    SPIFIFO.write(0);
    SPIFIFO.read();
    buf[i] = SPIFIFO.read();
	#endif
    #if 0
    // this does not work, but why?
    SPIFIFO.write16(0x0F00 | (addr >> 8), SPI_CONTINUE);
    SPIFIFO.write16(addr << 8);
    addr++;
    SPIFIFO.read();
    buf[i] = SPIFIFO.read();
    #endif
  }
  return len;
}
#else
uint16_t W5100Class::read(uint16_t _addr, uint8_t *_buf, uint16_t _len)
{
  for (uint16_t i=0; i<_len; i++)
  {
    setSS();
    SPI.transfer(0x0F);
    SPI.transfer(_addr >> 8);
    SPI.transfer(_addr & 0xFF);
    _addr++;
    _buf[i] = SPI.transfer(0);
    resetSS();
  }
  return _len;
}
#endif

void W5100Class::execCmdSn(SOCKET s, SockCMD _cmd) {
  // Send command to socket
  writeSnCR(s, _cmd);
  // Wait for command to complete
  while (readSnCR(s))
    ;
}
