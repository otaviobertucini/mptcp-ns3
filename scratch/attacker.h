#include <iostream>
#include <fstream>
#include <string>

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/mp-tcp-bulk-send-helper.h"
#include "ns3/mp-tcp-packet-sink-helper.h"

#include "ns3/config-store-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"

#include "ns3/event-id.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-classifier.h"
#include "ns3/flow-monitor-module.h"

#include "ns3/applications-module.h"
#include "ns3/ipv4-header.h"
#include "ns3/netanim-module.h"

using namespace ns3;

class Attacker : public Application
{
public:

  Attacker ();
  virtual ~Attacker();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

Attacker::Attacker ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

Attacker::~Attacker()
{
  m_socket = 0;
}

void
Attacker::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
Attacker::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
Attacker::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
Attacker::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  TcpHeader header;
  header.SetSourcePort(11111);
  packet->AddHeader(header);

  m_socket->Send (packet);

  //cout << "Atacante enviou pacote!" << endl;

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
Attacker::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &Attacker::SendPacket, this);
    }
}
