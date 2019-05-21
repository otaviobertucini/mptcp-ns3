/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 NR2, DINF, Federal University of Paraná
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Benevid Felix <benevid@gmail.com>
 *
 */

/*
 * This is a basic example that compares CoDel and DropTail queues using MpTCP topology:
 *
 *                    bottleneck
 *             0 <--------------------------> eNB <----------------------->
 *           /        10 Mb/s, 10 ms    droptail/codel       100 Mb/s, 5ms  \
 *     source                                                                ==> sink
 *           \                                                              /
 *            1 <--------------------------> WiFi <------------------------>
 *                  54 Mb/s, 5 ms      droptail       100 Mb/s, 5ms
 *
 *
 * The source generates traffic across the network using MpTCP Application.
 * The MpTCP is used as the transport-layer protocol.
 * Packets transmitted during a simulation run are captured into a .pcap file, and
 * congestion window values are also traced.
 */
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
#include "ns3/event-id.h"

#include "ns3/netanim-module.h"
#include "ns3/flow-classifier.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CodelDropTailMpTcpLteWifi");


void
PrintNodeInfo(NodeContainer nodes){
    
    Ptr< Node > node_p;
    Ptr< NetDevice > device_p;
    
    Ptr<Ipv4> ipv4;
    Ipv4InterfaceAddress iaddr;
    Ipv4Address m_ipAddr;
    Address m_addr;
    
    NS_LOG_INFO ("PrintNodeList");
    for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
        node_p = nodes.Get (i);
        std::string nodeName = Names::FindName (node_p);
        if(Names::FindName (node_p)!= "")
            NS_LOG_INFO ("NodeId: " << node_p->GetId () << " Name: " << nodeName);
        else
            NS_LOG_INFO ("NodeId: " << node_p->GetId () );
        // creates a new one, does not get the installed one.
        //Ptr<MobilityModel> mobility = node_p->GetObject <MobilityModel> ();
        //Vector pos = mobility->GetPosition ();
        //NS_LOG_INFO ("  Mobility Model: " << mobility->GetInstanceTypeId ());
        //NS_LOG_INFO ("  Position (x,y,z) [m]: " << pos.x << " " << pos.y << " " << pos.z);
        for (uint32_t j = 0; j < node_p->GetNDevices (); j++)
        {
            ipv4 = node_p->GetObject<Ipv4 > ();
            iaddr = ipv4->GetAddress (j, 0); //I/F, Address index
            m_ipAddr = iaddr.GetLocal ();
            
            device_p = node_p->GetDevice (j);
            m_addr = device_p->GetAddress ();
            NS_LOG_INFO ("  Device " << j << " IP " << m_ipAddr << " MAC " << m_addr);
        }
    }
    
}

static void
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream () << oldval << " " << newval << std::endl;
}

static void
TraceCwnd (std::string cwndTrFileName)
{
    AsciiTraceHelper ascii;
    if (cwndTrFileName.compare ("") == 0)
    {
        NS_LOG_DEBUG ("No trace file for cwnd provided");
        return;
    }
    else
    {
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (cwndTrFileName.c_str ());
        Config::ConnectWithoutContext ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",MakeBoundCallback (&CwndTracer, stream));
    }
}

void
printIterator(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter, Ptr<OutputStreamWrapper> output,
              const Ipv4FlowClassifier::FiveTuple &sFT)
{
    if (sFT.protocol == 17)
    {
        *output->GetStream() << "UDP: ";
    }
    if (sFT.protocol == 6)
    {
        *output->GetStream() << "TCP: ";
    }
    *output->GetStream() << "Flow " << iter->first << " (" << sFT.sourceAddress << " -> " << sFT.destinationAddress << ")\n";
    *output->GetStream() << "  Tx Bytes:   " << iter->second.txBytes << "\n";
    *output->GetStream() << "  Rx Bytes:   " << iter->second.rxBytes << "\n";
    *output->GetStream() << "  Packet Lost " << iter->second.lostPackets << "\n";
    *output->GetStream() << "  lastPacketSeen " << iter->second.timeLastRxPacket.GetSeconds() << "\n";
    *output->GetStream() << "  Rx Packets " << iter->second.rxPackets << "\n";
    *output->GetStream() << "  Tx Packets " << iter->second.txPackets << "\n";
    *output->GetStream() << "  Throughput: "
    << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds())
    / 1000 / 1000 << " Mbps\n";
    *output->GetStream() << "  Flow Completion Time: "
    << (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) << " s\n";
}

int
main(int argc, char *argv[]){
    
    
    uint16_t port = 8;
    uint32_t queueSize = 10;
    std::string queueType = "CoDel";       //DropTail or CoDel
    std::string linkType = "wifi";       //wifi or mmwave
    uint32_t pktSize = 1458;        //in bytes. 1458 to prevent fragments
    
    float startTime = 0.0;
    float simDuration = 10;        //in seconds
    float stopTime = startTime + simDuration;
    
    bool isPcapEnabled = false;
    std::string pcapFileName = "mptcp-lte.pcap";
    std::string cwndTrFileName = "cwndDropTail.tr";
    bool logging = true;
    
    CommandLine cmd;
    //cmd.AddValue ("bufsize", "Snd/Rcv buffer size.", bufSize);
    cmd.AddValue ("queueSize", "Queue size in packets", queueSize);
    cmd.AddValue ("pktSize", "Packet size in bytes", pktSize);
    cmd.AddValue ("queueType", "Queue type: DropTail, CoDel", queueType);
    cmd.AddValue ("logging", "Flag to enable/disable logging", logging);
    cmd.Parse (argc, argv);
    
    
    if (logging)
    {
        LogComponentEnable ("CodelDropTailMpTcpLteWifi", LOG_LEVEL_ALL);
        LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("MpTcpSocketBase", LOG_INFO);
        //LogComponentEnable("TcpHeader", LOG_ALL);
        //LogComponentEnable("Packet", LOG_ALL);
        //LogComponentEnable("TcpL4Protocol", LOG_ALL);
    }
    
    // Enable checksum
    if (isPcapEnabled)
    {
        //GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    }
    
    
    Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktSize));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(MpTcpSocketBase::GetTypeId()));
    Config::SetDefault("ns3::MpTcpSocketBase::MaxSubflows", UintegerValue(3)); // Sink
    /* Uncoupled_TCPs, Linked_Increases, RTT_Compensator, Fully_Coupled */
    //Config::SetDefault("ns3::MpTcpSocketBase::CongestionControl", StringValue("Uncoupled_TCPs"));
    
    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("DsssRate1Mbps"));
    
    // Configure the queue
    if (queueType.compare ("DropTail") == 0)
    {
        //LogComponentEnable ("DropTailQueue", LOG_LEVEL_ALL);
        Config::SetDefault("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
        Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(queueSize));
    }
    else if (queueType.compare ("CoDel") == 0)
    {
        //LogComponentEnable ("CoDelQueue", LOG_LEVEL_ALL);
        //LogComponentEnable ("CoDelQueue", LOG_LEVEL_FUNCTION);
        Config::SetDefault("ns3::CoDelQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
        Config::SetDefault("ns3::CoDelQueue::MaxPackets", UintegerValue(queueSize));
    }
    else
    {
        NS_LOG_DEBUG ("Invalid queue type");
        exit (1);
    }
    
    
    // Create a single RemoteHost
    Ptr<Node> hostRemote = CreateObject<Node> ();
    Names::Add ("remote", hostRemote);
    NodeContainer hostRemotesContainer = NodeContainer(hostRemote);
    
    Ptr<Node> hostRouter = CreateObject<Node> ();
    Names::Add ("router", hostRouter);
    NodeContainer hostRoutersContainer = NodeContainer(hostRouter);
    
    
    InternetStackHelper internet;
    internet.Install (hostRemote);
    internet.Install(hostRouter);
    
    
    Ptr<Node> ueNode = CreateObject<Node> ();
    
    NodeContainer ueNodes = NodeContainer(ueNode);
       // Install Mobility Model
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector(0, 5, 0));
    positionAlloc->Add (Vector(5, 25, 0));
    positionAlloc->Add (Vector(5, 20, 0));
    positionAlloc->Add (Vector(5, 5, 0));
    
    MobilityHelper mobilityEnb;
    mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityEnb.SetPositionAllocator(positionAlloc);
    mobilityEnb.Install(ueNodes);

    mobilityEnb.Install(hostRemotesContainer);
    mobilityEnb.Install(hostRoutersContainer);
    
    // Install the IP stack on the UEs
    internet.Install (ueNodes);
    
    Ptr<Node> apNode = CreateObject<Node> ();
    Names::Add ("wifiAp", apNode);
    NodeContainer apNodes = NodeContainer(apNode);
    
    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    
    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    // set it to zero; otherwise, gain will be added
    wifiPhy.Set ("RxGain", DoubleValue (-10) );
    // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
    
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel (wifiChannel.Create ());
    
    // Add a non-QoS upper mac, and disable rate control
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
    
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode",StringValue ("DsssRate1Mbps"),
                                  "ControlMode",StringValue ("DsssRate1Mbps"));
    // Set it to adhoc mode
    wifiMac.SetType ("ns3::AdhocWifiMac");
    
    NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, NodeContainer(ueNode,apNode));
    mobilityEnb.Install (apNode);
    
    //NetDeviceContainer ueWifiDevice = wifi.Install (phy, mac, ueNode);
    
    /* Internet stack*/
    internet.Install (apNode);
    
    // -------------------------------------------------------------------------------
    //  P2P CONFIGURATION
    // -------------------------------------------------------------------------------
    
    Ipv4AddressHelper ipv4h;
    
    // Point-to-point links
    NodeContainer nAp2router = NodeContainer (apNode, hostRouter);
    NodeContainer nRouter2sink = NodeContainer (hostRouter,hostRemote);
    
    //p2p for pgw to router
    PointToPointHelper p2pGw2router;
    p2pGw2router.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Mb/s")));
    p2pGw2router.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2pGw2router.SetChannelAttribute ("Delay", TimeValue (Seconds (0.001)));   //0.010
    
    //p2p for AP to roter
    PointToPointHelper p2pAp2router;
    p2pAp2router.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Mb/s")));
    p2pAp2router.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2pAp2router.SetChannelAttribute ("Delay", TimeValue (Seconds (0.030)));   //0.010
    
    PointToPointHelper p2pRouter2sink;
    p2pRouter2sink.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gb/s")));
    p2pRouter2sink.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2pRouter2sink.SetChannelAttribute ("Delay", TimeValue (Seconds (0.0001)));
    
    //netdevices
    NetDeviceContainer dAp2router = p2pAp2router.Install (nAp2router);
    NetDeviceContainer dRouter2sink = p2pRouter2sink.Install (nRouter2sink);
    
    // ap to router
    ipv4h.SetBase("10.10.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifAp2router = ipv4h.Assign (dAp2router);
    // router to sink
    ipv4h.SetBase("10.20.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifRouter2sink = ipv4h.Assign (dRouter2sink);
    
    // ue to ap
    NetDeviceContainer dUe2Ap = NetDeviceContainer(apDevice);
    ipv4h.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifUe2Ap = ipv4h.Assign (dUe2Ap);
    ipv4h.NewNetwork ();
    
    
    // -------------------------------------------------------------------------------
    //  STATIC ROUTING CONFIGURATION
    // -------------------------------------------------------------------------------
    
    Ptr<Ipv4> ipv4Ue = ueNode->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Router = hostRouter->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Remote = hostRemote->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4SAp = apNode->GetObject<Ipv4> ();
    
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Ue);
    Ptr<Ipv4StaticRouting> apStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4SAp);
    Ptr<Ipv4StaticRouting> routerStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Router);
    Ptr<Ipv4StaticRouting> remoteStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Remote);
    /*
     * @dest 	The Ipv4Address destination for this route.
     * @nextHop 	The Ipv4Address of the next hop in the route.
     * @interface 	The network interface index used to send packets to the destination.
     * @metric 	Metric of route in case of multiple routes to same destination
     */
    //ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.1.2.2"), 1);
    apStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.10.2.2"), 1);
    
    //routerStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.20.1.2"), 2);
    routerStaticRouting->AddHostRouteTo (Ipv4Address ("10.1.2.1"), Ipv4Address ("10.10.2.1"), 1);
    //remote to UE
    remoteStaticRouting->AddHostRouteTo (Ipv4Address ("10.1.2.1"), Ipv4Address ("10.20.1.1"), 1);
    
    /* Generate Route. */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    
    // -------------------------------------------------------------------------------
    //  QUEUE CONFIGURATION
    // -------------------------------------------------------------------------------
    
    // Configure the queue
    if (queueType.compare ("DropTail") == 0)
    {
        p2pRouter2sink.SetQueue ("ns3::DropTailQueue",
                                 "Mode", StringValue ("QUEUE_MODE_PACKETS"),
                                 "MaxPackets", UintegerValue (queueSize));
    }
    else if (queueType.compare ("CoDel") == 0)
    {
        p2pRouter2sink.SetQueue ("ns3::CoDelQueue",
                                 "Mode", StringValue ("QUEUE_MODE_PACKETS"),
                                 "MaxPackets", UintegerValue (queueSize));
    }
    else
    {
        NS_LOG_DEBUG ("Invalid queue type");
        exit (1);
    }
    
    // -------------------------------------------------------------------------------
    //  APP CONFIGURATION
    // -------------------------------------------------------------------------------
    PrintNodeInfo(ueNodes);
    PrintNodeInfo(apNodes);
    PrintNodeInfo(hostRoutersContainer);
    PrintNodeInfo(hostRemotesContainer);
    
    NS_LOG_INFO ("Create Applications.");
    
    MpTcpPacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(hostRemote);
    sinkApps.Start(Seconds(startTime));
    sinkApps.Stop(Seconds(stopTime-1));
    
    Ipv4Address dstaddr ("10.20.1.2");
    
    MpTcpBulkSendHelper sourceHelper("ns3::TcpSocketFactory", InetSocketAddress(dstaddr, port));
    sourceHelper.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApp = sourceHelper.Install(ueNode);
    sourceApp.Start(Seconds(startTime));
    sourceApp.Stop(Seconds(stopTime-1));
    
    AnimationInterface anim ("simple2.xml");
    //AnimationInterface::SetNodeColor(ueNode, 0, 0, 205);
    anim.UpdateNodeColor(ueNode, 0, 0, 205);
    anim.UpdateNodeDescription(ueNode, "ueNode");
    anim.UpdateNodeColor(hostRemote, 0, 100, 100);
    anim.UpdateNodeDescription(hostRemote, "sinkNode");
    anim.UpdateNodeDescription(apNode, "apWifi");
    anim.UpdateNodeDescription(hostRouter, "Router");
    
    
    //Simulator::Schedule (Seconds (0.0001), &TraceCwnd, cwndTrFileName);
    
    AsciiTraceHelper h_ascii;
    /* Flow Monitor Configuration */
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (Seconds(stopTime));
    Simulator::Run();
    
    monitor->CheckForLostPackets();
    
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter;
    Ptr<OutputStreamWrapper> output = h_ascii.CreateFileStream("mptcp2.Results");
    for (iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        
        if ((t.sourceAddress == "10.1.2.1" && t.destinationAddress == "10.20.1.2"))
        {
            printIterator(iter, output, t);
        }
        else{
            printIterator(iter, output, t);
        }
    }
    monitor->SerializeToXmlFile("mptcp2.flowmon", false, false);
    
    
    if (isPcapEnabled)
    {
        p2pGw2router.EnablePcapAll ("mptcp-lte2.pcap",false);
        p2pAp2router.EnablePcapAll ("mptcp-wifi2.pcap", false);
        
    }
    
    Simulator::Destroy();
    NS_LOG_INFO ("Done.");
    
}
