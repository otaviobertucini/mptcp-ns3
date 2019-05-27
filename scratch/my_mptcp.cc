/*
Topologia:

  ---- m1 ------
s               d
  ---- m2 ------

*/

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

#include "ns3/attacker-application.h"
// class Attacker;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MyMpTcp");

int
main(int argc, char *argv[]){

    LogComponentEnable("MpTcpSocketBase", LOG_INFO);

    Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
    Config::SetDefault("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(100));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(MpTcpSocketBase::GetTypeId()));
    Config::SetDefault("ns3::MpTcpSocketBase::MaxSubflows", UintegerValue(8)); // Sink
    //Config::SetDefault("ns3::MpTcpSocketBase::CongestionControl", StringValue("RTT_Compensator"));
    //Config::SetDefault("ns3::MpTcpSocketBase::PathManagement", StringValue("NdiffPorts"));


    // Create a receiver node
    Ptr<Node> receiverHost = CreateObject<Node> ();
    Names::Add ("receiver", receiverHost);
    NodeContainer receiverHostsContainer = NodeContainer(receiverHost);

    /* TCP Node (sender node) */
    Ptr<Node> sourceNode = CreateObject<Node> ();
    Names::Add ("sender", sourceNode);
    NodeContainer sourceContainer = NodeContainer(sourceNode);

    // Create a middle node
    Ptr<Node> m1 = CreateObject<Node> ();
    Names::Add ("node1", m1);
    NodeContainer m1sContainer = NodeContainer(m1);

    // Create another middle node
    Ptr<Node> m2 = CreateObject<Node> ();
    Names::Add ("node2", m2);
    NodeContainer m2sContainer = NodeContainer(m2);

    //Install internet stack
    InternetStackHelper internet;
    internet.Install (receiverHost);
    internet.Install(sourceNode); //TCP node
    internet.Install(m1);
    internet.Install(m2);

    //Create the containers for each node linked
    NodeContainer nTcp_m1 = NodeContainer(sourceNode, m1); //sender to m1
    NodeContainer nTcp_m2 = NodeContainer(sourceNode, m2); //sender to m2

    //m1 to receiver
    NodeContainer nM1_receiver = NodeContainer(m1, receiverHost);

    //m2 to receiver
    NodeContainer nM2_receiver = NodeContainer(m2, receiverHost);

    //Make the links among the nodes
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer linkTcp_m1 = pointToPoint.Install(nTcp_m1);
    NetDeviceContainer linkTcp_m2 = pointToPoint.Install(nTcp_m2);
    NetDeviceContainer linkM1_receiver = pointToPoint.Install(nM1_receiver);
    NetDeviceContainer linkM2_receiver = pointToPoint.Install(nM2_receiver);

    //IPV4 settings
    Ipv4AddressHelper ipv4;

    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iplinkTcp_m1 = ipv4.Assign (linkTcp_m1);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer iplinkTcp_m2 = ipv4.Assign (linkTcp_m2);

    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer iplinkM1_receiver = ipv4.Assign (linkM1_receiver);

    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer iplinkM2_receiver = ipv4.Assign (linkM2_receiver);

    //MPTCP
    uint16_t port = 9;
    MpTcpPacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(receiverHost);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(2.0));

    //Ipv4Address dstaddr ("10.20.1.2");

    MpTcpBulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address(iplinkM2_receiver.GetAddress(1)), port));
    source.SetAttribute("MaxBytes", UintegerValue(4194304));
    ApplicationContainer sourceApps = source.Install(sourceNode);
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(2.0));

    // Create attacker node
    Ptr<Node> attackerNode = CreateObject<Node> ();
    Names::Add ("attacker", attackerNode);
    NodeContainer attackerHostContainer = NodeContainer(attackerNode);
    internet.Install(attackerNode);

    // Ptr<Socket> sock = Socket::CreateSocket(atttackerNode,
    //   TcpSocketFactory::GetTypeId());
    // Ptr<AttackerSocket> sock = CreateObject<AttackerSocket>();
    // Ptr<AttackerSocket> sock = DynamicCast<AttackerSocket>
    //     (Socket::CreateSocket(attackerNode, TcpSocketFactory::GetTypeId()));

    //TODO: the m_tcp atribute of the socket is not being created. See whats
    // going on. Problaby the socket is not being created on the right mode.

    // Connect attacker and source
    //attacker to sender
    NodeContainer nAtck_src = NodeContainer(attackerNode, sourceNode);
    PointToPointHelper p2pAttacker;
    p2pAttacker.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pAttacker.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer linkAtck_src = pointToPoint.Install(nAtck_src);

    ipv4.NewNetwork ();
    ipv4.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iplinkAtck_src = ipv4.Assign (linkAtck_src);

    //uint16_t sourcePort = 8080;
    Ptr<Attacker> attacker = CreateObject<Attacker>();
    attackerNode->AddApplication(attacker);
    attacker->Setup (InetSocketAddress(iplinkAtck_src.GetAddress(0),
                    port), 1040, 1000, DataRate("1Mbps"));
    attacker->SetStartTime(Seconds(1.5));
    attacker->SetStopTime(Seconds(3.0));

	   Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	   //AnimationInterface anim("my_mptcp.xml");

    Simulator::Stop (Seconds(4.0));
    Simulator::Run();

    Simulator::Destroy();
    NS_LOG_INFO ("Simulation done.");
}
