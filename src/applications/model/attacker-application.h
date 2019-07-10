#ifndef ATTACKER_H
#define ATTACKER_H

// #include "ns3/attacker-socket.h"
// #include "ns3/attacker-socket-factory.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include "ns3/application.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-socket-base.h"

using namespace std;

namespace ns3{

class AttackerSocket;

class Attacker : public Application
{
public:

  Attacker ();
  virtual ~Attacker();

  void Setup (Address address, uint32_t packetSize,
              uint32_t nPackets, DataRate dataRate, Address my_addres,
              Ipv4Address address4, Ipv4Address my_address4,
              uint32_t port);

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);
  void Receive(Ptr<Socket> socket);

  static TypeId GetTypeId (void);

  Ptr<TcpSocketBase>     m_socket;
  Address         m_peer;
  Address         m_myAddress;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  Ipv4Address     m_peer4;
  Ipv4Address     m_myAddress4;
  uint32_t        peer_port;

private:

};

}

#endif
