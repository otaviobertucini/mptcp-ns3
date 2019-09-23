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
                     DataRate dataRate, Address my_address,
                     Ipv4Address address4, Ipv4Address my_address4,
                     uint32_t port)
    {

      // m_socket = socket;
      m_peer = address;
      m_packetSize = packetSize;
      m_nPackets = nPackets;
      m_dataRate = dataRate;
      m_myAddress = my_address;
      m_peer4 = address4;
      m_myAddress4 = my_address4;
      peer_port = port;

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

      // m_socket->Bind ();
      // std::cout << "AFTER BIND" << std::endl;
      //
      if(m_socket->Connect2 (m_peer) == 0){
        std::cout << "ATTACKER SOCKET CONECTADO" << std::endl;
      }

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
      // Ptr<Packet> packet = Create<Packet> (m_packetSize);

      TcpHeader header;

      // header.SetSourcePort(sFlow->sPort);
      // header.SetDestinationPort(sFlow->dPort);
      // header.SetFlags(TcpHeader::ACK);
      header.SetSequenceNumber(SequenceNumber32(111));
      header.SetSourcePort(peer_port);
      header.SetDestinationPort(49153);
      // header.SetAckNumber(SequenceNumber32(sFlow->RxSeqNumber));
      // header.SetWindowSize(AdvertisedWindowSize());
      // packet->AddHeader(header);

      uint32_t localToken = rand();

      header.SetFlags(TcpHeader::SYN | TcpHeader::ACK);

      header.AddOptMPC(OPT_MPC, localToken);
      header.SetLength(7);
      header.SetOptionsLength(2);
      header.SetPaddingLength(3);

      m_socket->SendPacket(header, m_peer4, m_myAddress4);

      TcpHeader header2;
      header2.SetSequenceNumber(SequenceNumber32(111));
      header2.SetSourcePort(peer_port);
      header2.SetDestinationPort(49153);
      header2.SetFlags(TcpHeader::ACK);
      MpTcpAddressInfo* addrInfo = new MpTcpAddressInfo();
      addrInfo->addrID = 0;
      addrInfo->ipv4Addr = m_myAddress4;
      // addrInfo->mask = interfaceAddr.GetMask();
      header.AddOptADDR(OPT_ADDR, addrInfo->addrID, addrInfo->ipv4Addr);
      header.SetLength(7);
      header.SetOptionsLength(2);
      header.SetPaddingLength(2);

      // m_socket->SendPacket(header2, m_peer4, m_myAddress4);

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
        std::cout << "PACOTE RECEBIDO PELO ATACANTE" << std::endl;
    }

}
