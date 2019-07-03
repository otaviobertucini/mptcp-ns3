#ifndef ATTACKER_H
#define ATTACKER_H

#include "ns3/attacker-socket.h"
// #include "ns3/attacker-socket-factory.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include "ns3/application.h"
#include "ns3/tcp-header.h"

using namespace std;

namespace ns3{

class AttackerSocket;

class Attacker : public Application
{
public:

  Attacker ();
  virtual ~Attacker();

  void Setup (Address address, uint32_t packetSize,
              uint32_t nPackets, DataRate dataRate);

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);
  void Receive(Ptr<Socket> socket);

  static TypeId GetTypeId (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;

private:

};

}

#endif
