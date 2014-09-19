/*
 modified 12 Aug 2013
 by Soohwan Kim (suhwan@wiznet.co.kr)
*/
#include "wiznet.h"
#include "Ethernet.h"
#include "Dhcp.h"
#include "util.h"

// XXX: don't make assumptions about the value of MAX_SOCK_NUM.
uint8_t EthernetClass::_state[MAX_SOCK_NUM] = { 
  0, 0, 0, 0 };
uint16_t EthernetClass::_server_port[MAX_SOCK_NUM] = { 
  0, 0, 0, 0 };

int EthernetClass::begin(uint8_t *mac_address)
{
  static DhcpClass s_dhcp;
  _dhcp = &s_dhcp;


  // Initialise the basic info
  Wiznet.init();
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  Wiznet.setMACAddress(mac_address);
  Wiznet.setIPAddress(IPAddress(0,0,0,0).raw_address());
  SPI.endTransaction();

  // Now try to get our config info from a DHCP server
  int ret = _dhcp->beginWithDHCP(mac_address);
  if(ret == 1)
  {
    // We've successfully found a DHCP server and got our configuration info, so set things
    // accordingly
    SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
    Wiznet.setIPAddress(_dhcp->getLocalIp().raw_address());
    Wiznet.setGatewayIp(_dhcp->getGatewayIp().raw_address());
    Wiznet.setSubnetMask(_dhcp->getSubnetMask().raw_address());
    SPI.endTransaction();
    _dnsServerAddress = _dhcp->getDnsServerIp();
  }

  return ret;
}

void EthernetClass::begin(uint8_t *mac_address, IPAddress local_ip)
{
  // Assume the DNS server will be the machine on the same network as the local IP
  // but with last octet being '1'
  IPAddress dns_server = local_ip;
  dns_server[3] = 1;
  begin(mac_address, local_ip, dns_server);
}

void EthernetClass::begin(uint8_t *mac_address, IPAddress local_ip, IPAddress dns_server)
{
  // Assume the gateway will be the machine on the same network as the local IP
  // but with last octet being '1'
  IPAddress gateway = local_ip;
  gateway[3] = 1;
  begin(mac_address, local_ip, dns_server, gateway);
}

void EthernetClass::begin(uint8_t *mac_address, IPAddress local_ip, IPAddress dns_server, IPAddress gateway)
{
  IPAddress subnet(255, 255, 255, 0);
  begin(mac_address, local_ip, dns_server, gateway, subnet);
}

void EthernetClass::begin(uint8_t *mac, IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet)
{
  Wiznet.init();
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  Wiznet.setMACAddress(mac);
  Wiznet.setIPAddress(local_ip._address);
  Wiznet.setGatewayIp(gateway._address);
  Wiznet.setSubnetMask(subnet._address);
  SPI.endTransaction();
  _dnsServerAddress = dns_server;
}

#if defined(USE_BURNED_MACADDRESS)
int EthernetClass::begin(void)
{
  byte mac_address[6] ={0,};
  _dhcp = new DhcpClass();


  // Initialise the basic info
  Wiznet.init();
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  Wiznet.setIPAddress(IPAddress(0,0,0,0).raw_address());
  Wiznet.getMACAddress(mac_address);
  SPI.endTransaction();

  WIZNET_DEBUG("MAC Address: ");
  WIZNET_DEBUG(mac_address[0], HEX);
  WIZNET_DEBUG(":");
  WIZNET_DEBUG(mac_address[1], HEX);
  WIZNET_DEBUG(":");
  WIZNET_DEBUG(mac_address[2], HEX);
  WIZNET_DEBUG(":");
  WIZNET_DEBUG(mac_address[3], HEX);
  WIZNET_DEBUG(":");
  WIZNET_DEBUG(mac_address[4], HEX);
  WIZNET_DEBUG(":");
  WIZNET_DEBUG(mac_address[5], HEX);
  WIZNET_DEBUGLN();

  // Now try to get our config info from a DHCP server
  int ret = _dhcp->beginWithDHCP(mac_address);

  WIZNET_DEBUG("DHCP Return: ");
  WIZNET_DEBUGLN(ret);

  if(ret == 1)
  {
    // We've successfully found a DHCP server and got our configuration info, so set things
    // accordingly
    SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
    Wiznet.setIPAddress(_dhcp->getLocalIp().raw_address());
    Wiznet.setGatewayIp(_dhcp->getGatewayIp().raw_address());
    Wiznet.setSubnetMask(_dhcp->getSubnetMask().raw_address());
    SPI.endTransaction();
    _dnsServerAddress = _dhcp->getDnsServerIp();
  }

  return ret;
}

void EthernetClass::begin(IPAddress local_ip)
{
  // Assume the DNS server will be the machine on the same network as the local IP
  // but with last octet being '1'
  IPAddress dns_server = local_ip;
  dns_server[3] = 1;
  begin(local_ip, dns_server);
}

void EthernetClass::begin(IPAddress local_ip, IPAddress dns_server)
{
  // Assume the gateway will be the machine on the same network as the local IP
  // but with last octet being '1'
  IPAddress gateway = local_ip;
  gateway[3] = 1;
  begin(local_ip, dns_server, gateway);
}

void EthernetClass::begin(IPAddress local_ip, IPAddress dns_server, IPAddress gateway)
{
  IPAddress subnet(255, 255, 255, 0);
  begin(local_ip, dns_server, gateway, subnet);
}

void EthernetClass::begin(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet)
{
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  Wiznet.init();
  Wiznet.setIPAddress(local_ip._address);
  Wiznet.setGatewayIp(gateway._address);
  Wiznet.setSubnetMask(subnet._address);
  SPI.endTransaction();
  _dnsServerAddress = dns_server;
}
#endif

int EthernetClass::maintain(){
  int rc = DHCP_CHECK_NONE;
  if(_dhcp != NULL){
    //we have a pointer to dhcp, use it
    rc = _dhcp->checkLease();
    switch ( rc ){
      case DHCP_CHECK_NONE:
        //nothing done
        break;
      case DHCP_CHECK_RENEW_OK:
      case DHCP_CHECK_REBIND_OK:
        //we might have got a new IP.
        SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
        Wiznet.setIPAddress(_dhcp->getLocalIp().raw_address());
        Wiznet.setGatewayIp(_dhcp->getGatewayIp().raw_address());
        Wiznet.setSubnetMask(_dhcp->getSubnetMask().raw_address());
        SPI.endTransaction();
        _dnsServerAddress = _dhcp->getDnsServerIp();
        break;
      default:
        //this is actually a error, it will retry though
        break;
    }
  }
  return rc;
}

IPAddress EthernetClass::localIP()
{
  IPAddress ret;
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  Wiznet.getIPAddress(ret.raw_address());
  SPI.endTransaction();
  return ret;
}

IPAddress EthernetClass::subnetMask()
{
  IPAddress ret;
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  Wiznet.getSubnetMask(ret.raw_address());
  SPI.endTransaction();
  return ret;
}

IPAddress EthernetClass::gatewayIP()
{
  IPAddress ret;
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  Wiznet.getGatewayIp(ret.raw_address());
  SPI.endTransaction();
  return ret;
}

IPAddress EthernetClass::dnsServerIP()
{
  return _dnsServerAddress;
}

EthernetClass Ethernet;
