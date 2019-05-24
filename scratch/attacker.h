#ifndef ATTACKER_H
#define ATTACKER_H

#include "attacker_socket.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include "ns3/application.h"

using namespace std;

namespace ns3{

class Attacker : public Application
{
public:

  Attacker ();
  virtual ~Attacker();

  void Setup (Ptr<AttackerSocket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<AttackerSocket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

}

#endif
