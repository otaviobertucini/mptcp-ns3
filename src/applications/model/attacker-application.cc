#include "attacker-application.h"

using namespace std;

namespace ns3{

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
Attacker::Setup (Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  // m_socket = socket;
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
  m_socket = DynamicCast<AttackerSocket>(Socket::CreateSocket(GetNode(),
              AttackerSocketFactory::GetTypeId()));
  m_socket->Bind ();
  // if(m_socket->Connect (m_peer) == -1){
  //   cout << "-------------------------" << endl;
  //   cout << "Não deu" << endl;
  //   cout << "-------------------------" << endl;
  // }
  //SendPacket ();
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

  //m_socket->Send (packet);

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

TypeId
Attacker::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Attacker")
    .SetParent<Application> ()
    .SetGroupName ("Application")
    ;
  return tid;
}

}
