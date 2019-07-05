#include "attacker-application.h"

using namespace std;

namespace ns3{

    NS_OBJECT_ENSURE_REGISTERED (Attacker);

      TypeId
      Attacker::GetTypeId (void)
      {
        static TypeId tid = TypeId ("ns3::Attacker")
          .SetParent<Application> ()
          .AddConstructor<Attacker>()
          .SetGroupName ("Application")
          ;
        return tid;
      }

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
    Attacker::Setup (Address address, uint32_t packetSize, uint32_t nPackets,
                      DataRate dataRate, Address my_address)
    {
      // m_socket = socket;
      m_peer = address;
      m_packetSize = packetSize;
      m_nPackets = nPackets;
      m_dataRate = dataRate;
      m_myAddress = my_address;

      // cout << "ADDRESS IS: " << address << endl;
    }

    void
    Attacker::StartApplication (void)
    {
      m_running = true;
      m_packetsSent = 0;
      // m_socket = DynamicCast<AttackerSocket>(Socket::CreateSocket(GetNode(),
      //             TcpSocketFactory::GetTypeId()));

      Ptr<Socket> aux = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId());
      // std::cout << "TYPE BEFORE DYNAMIC my: " << aux << std::endl;

      m_socket = DynamicCast<TcpSocketBase>(aux);
      //
      // std::cout << "TYPE AFTER DYNAMIC: " << m_socket << std::endl;

      // cout << "SOCKET: " << m_socket->GetTypeId() << endl;
      // cout << "SOCKET: " << m_socket << endl;

      m_socket->Bind ();
      std::cout << "AFTER BIND" << std::endl;

      // if(m_socket->Connect (m_peer) == 0){
      //   std::cout << "ATTACKER SOCKET CONECTADO" << std::endl;
      // }

      m_socket->SetRecvCallback( MakeCallback(&Attacker::Receive, this) );

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

      // header.SetSourcePort(sFlow->sPort);
      // header.SetDestinationPort(sFlow->dPort);
      header.SetFlags(TcpHeader::ACK);
      header.SetSequenceNumber(SequenceNumber32(1));
      // header.SetAckNumber(SequenceNumber32(sFlow->RxSeqNumber));
      // header.SetWindowSize(AdvertisedWindowSize());
      packet->AddHeader(header);

      // m_socket->Send (packet);
      m_socket->SendPacket(header, m_peer, m_myAddress);
      cout << "Atacante enviou pacote!" << endl;
      cout << "uid: " << packet->GetUid() << endl;

      // if (++m_packetsSent < m_nPackets)
      //   {
      //     ScheduleTx ();
      //   }
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

    void Attacker::Receive(Ptr<Socket> socket){
        std::cout << "packet received" << std::endl;
    }

}
