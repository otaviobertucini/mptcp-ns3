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
#include "ns3/event-id.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-classifier.h"
#include "ns3/flow-monitor-module.h"

#include "ns3/applications-module.h"

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

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset dts1, Gnuplot2dDataset dts2)
{
    double localThrou=0;
    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
    
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
        Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
        
        localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
        
        if ((fiveTuple.sourceAddress == "7.0.0.2" && fiveTuple.destinationAddress == "10.20.1.2"))
        {
            if ((double)localThrou > 0)
                dts1.Add((double)Simulator::Now().GetSeconds(),(double)localThrou);
        }
        if ((fiveTuple.sourceAddress == "10.1.2.1" && fiveTuple.destinationAddress == "10.20.1.2"))
        {
            if ((double)localThrou > 0)
                dts2.Add((double)Simulator::Now().GetSeconds(),(double)localThrou);
        }
        
        
        
    }
    Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon, dts1, dts2);
    flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
    
    
}

void PlotThroughput(Gnuplot2dDataset dat1,Gnuplot2dDataset dat2){
    
    std::string fileNameWithNoExtension = "FlowVSThroughput_";
    std::string graphicsFileName        = fileNameWithNoExtension + ".eps";
    std::string plotFileName            = fileNameWithNoExtension + ".plt";
    std::string plotTitle               = "Flow vs Throughput";
    std::string dataTitle1               = "fluxo 1(LTE)";
    std::string dataTitle2               = "fluxo 2(WIFI)";
    
    Gnuplot gnuplot (graphicsFileName);
    /*
    gnuplot.AppendExtra("set terminal postscript eps enhanced color solid font 'Helvetica,15'\n"
                                "set output \"throughput.eps\"\n"
                                "set xlabel \"Flows\" offset 0,-1\n"
                                "set ylabel \"Throughput(Mbs)\" offset 0,0\n"
                                "set grid\n"
                                "set lmargin 10.0\n"
                                "set rmargin 5.0\n"
                                "set key top left horizontal Left reverse noenhanced autotitles columnhead\n");
    */
    gnuplot.SetTitle (plotTitle);
    
    
    // Make the graphics file, which the plot file will be when it
    // is used with Gnuplot, be a PNG file.
    gnuplot.SetTerminal ("eps");
    
    // Set the labels for each axis.
    gnuplot.SetLegend ("Tempo(s)", "Throughput(Mbps)");
    
    //Gnuplot2dDataset dataset;
    dat1.SetTitle (dataTitle1);
    dat2.SetTitle (dataTitle2);
    dat1.SetStyle (Gnuplot2dDataset::LINES_POINTS);
    dat2.SetStyle (Gnuplot2dDataset::LINES_POINTS);

    //Gnuplot ...continued
    gnuplot.AddDataset (dat1);
    gnuplot.AddDataset (dat2);
    // Open the plot file.
    std::ofstream plotFile (plotFileName.c_str());
    // Write the plot file.
    gnuplot.GenerateOutput (plotFile);
    // Close the plot file.
    plotFile.close ();
}

int
main(int argc, char *argv[]){


    uint16_t port = 8;
    uint32_t queueSize = 1000;
    std::string queueType = "DropTail";       //DropTail or CoDel
    std::string linkType = "wifi";       //wifi or mmwave
    uint32_t pktSize = 1458;        //in bytes. 1458 to prevent fragments

    float startTime = 0.0;
    float simDuration = 10;        //in seconds
    float stopTime = startTime + simDuration;

    bool isPcapEnabled = false;
    std::string pcapFileName = "mptcp-lte.pcap";
    std::string cwndTrFileName = "cwndDropTail.tr";
    bool logging = false;

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
    //Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));
    
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktSize));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(MpTcpSocketBase::GetTypeId()));
    Config::SetDefault("ns3::MpTcpSocketBase::MaxSubflows", UintegerValue(8)); // Sink
    /* Uncoupled_TCPs, Linked_Increases, RTT_Compensator, Fully_Coupled */
    Config::SetDefault("ns3::MpTcpSocketBase::CongestionControl", StringValue("Fully_Coupled"));
    //Config::SetDefault("ns3::MpTcpSocketBase::PathManagement", StringValue("FullMesh"));
    Config::SetDefault("ns3::MpTcpSocketBase::LargePlotting", BooleanValue(true));
    
    // disable fragmentation for frames below 2200 bytes
    //Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
    //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    //Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("DsssRate1Mbps"));
   
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
    Names::Add ("ueNode", ueNode);
    
    Ptr<Node> enbNode = CreateObject<Node> ();
    Names::Add ("enbNode", enbNode);
    
    NodeContainer ueNodes = NodeContainer(ueNode);
    NodeContainer enbNodes = NodeContainer(enbNode);
    
    // Install Mobility Model
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector(0, 10, 0));
    positionAlloc->Add (Vector(0, 15, 0));
    positionAlloc->Add (Vector(0, 20, 0));
    positionAlloc->Add (Vector(10, 30, 0));
    positionAlloc->Add (Vector(10, 25, 0));
    positionAlloc->Add (Vector(10, 10, 0));
    
    MobilityHelper mobilityEnb;
    mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityEnb.SetPositionAllocator(positionAlloc);
    
    // -------------------------------------------------------------------------------
    //  WIFI NETWORK CONFIGURATION
    // -------------------------------------------------------------------------------
    
    
    //Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    
    Ptr<Node> apNode = CreateObject<Node> ();
    Names::Add ("wifiAp", apNode);
    NodeContainer apNodes = NodeContainer(apNode);
    
    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi = WifiHelper::Default ();
    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
    
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                                    "Exponent", DoubleValue (3.0));
    wifiPhy.SetChannel (wifiChannel.Create ());
    
    // Add a non-QoS upper mac, and disable rate control
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
 
    // Set it to adhoc mode
    wifiMac.SetType ("ns3::AdhocWifiMac");
    
    /*I will have 22.915Mbps throughput.
    both cases(54Mbps) you should be able to obtain 30Mbps with the OnOffApplication for 1500 byte packet size
    
    */ 
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode",StringValue ("OfdmRate54Mbps"),
                                  "ControlMode",StringValue ("OfdmRate54Mbps"));

    
    
    NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, NodeContainer(ueNode,apNode));
    
    /* Internet stack*/
    internet.Install (apNode);
    internet.Install (ueNode);

    
    Ipv4AddressHelper ipv4h;
    // ue to ap
    NetDeviceContainer dUe2Ap = NetDeviceContainer(apDevice);
    ipv4h.SetBase("10.1.2.0", "255.255.255.0");
     Ipv4InterfaceContainer ifUe2Ap = ipv4h.Assign (dUe2Ap);
    ipv4h.NewNetwork ();
    
    
    // -------------------------------------------------------------------------------
    //  LTE NETWORK CONFIGURATION
    // -------------------------------------------------------------------------------

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
   
    Ptr<Node> gwLte = epcHelper->GetPgwNode ();
    Names::Add ("pGw", gwLte);
    
    mobilityEnb.Install(ueNodes);
    mobilityEnb.Install(enbNodes);
    mobilityEnb.Install(gwLte);
    mobilityEnb.Install(hostRemotesContainer);
    mobilityEnb.Install(hostRoutersContainer);
    mobilityEnb.Install (apNode);
    
    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

    // Install the IP stack on the UEs
    
    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
   
    // Attach one UE per eNodeB
    lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));

    // -------------------------------------------------------------------------------
    //  P2P CONFIGURATION
    // -------------------------------------------------------------------------------
    
    
    // Point-to-point links
    NodeContainer nGw2router = NodeContainer (gwLte, hostRouter);
    NodeContainer nAp2router = NodeContainer (apNode, hostRouter);
    NodeContainer nRouter2sink = NodeContainer (hostRouter,hostRemote);

    //p2p for pgw to router
    PointToPointHelper p2pGw2router;
    p2pGw2router.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gb/s")));
    p2pGw2router.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2pGw2router.SetChannelAttribute ("Delay", TimeValue (Seconds (0.001)));   //0.010
    
    //p2p for AP to roter
    PointToPointHelper p2pAp2router;
    p2pAp2router.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("150Gb/s")));
    p2pAp2router.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2pAp2router.SetChannelAttribute ("Delay", TimeValue (Seconds (0.001)));   //0.010
    
    PointToPointHelper p2pRouter2sink;
    p2pRouter2sink.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("150Gb/s")));
    p2pRouter2sink.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2pRouter2sink.SetChannelAttribute ("Delay", TimeValue (Seconds (0.001)));
    
    //netdevices
    NetDeviceContainer dGw2router = p2pGw2router.Install (nGw2router);
    NetDeviceContainer dAp2router = p2pAp2router.Install (nAp2router);
    NetDeviceContainer dRouter2sink = p2pRouter2sink.Install (nRouter2sink);
    
    //pgw to router
    ipv4h.SetBase ("10.10.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifGw2router = ipv4h.Assign (dGw2router);
    // ap to router
    ipv4h.SetBase("10.10.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifAp2router = ipv4h.Assign (dAp2router);
    ipv4h.NewNetwork ();
    // router to sink
    ipv4h.SetBase("10.20.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifRouter2sink = ipv4h.Assign (dRouter2sink);
    
    
    // -------------------------------------------------------------------------------
    //  STATIC ROUTING CONFIGURATION
    // -------------------------------------------------------------------------------
    
    Ptr<Ipv4> ipv4Ue = ueNode->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Router = hostRouter->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Remote = hostRemote->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4pgw = gwLte->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4SAp = apNode->GetObject<Ipv4> ();
    
    
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Ue);
    Ptr<Ipv4StaticRouting> gwStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4pgw);
    Ptr<Ipv4StaticRouting> apStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4SAp);
    Ptr<Ipv4StaticRouting> routerStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Router);
    Ptr<Ipv4StaticRouting> remoteStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Remote);
    /*
     * @dest 	The Ipv4Address destination for this route.
     * @nextHop 	The Ipv4Address of the next hop in the route.
     * @interface 	The network interface index used to send packets to the destination.
     * @metric 	Metric of route in case of multiple routes to same destination
     */
    /* Generate Route. */
    //routerStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.20.1.0"), Ipv4Mask ("255.255.255.0"), 2);
    ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.1.2.2"), 1,2);
    ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("7.0.0.1"), 2,2);
    
    //AsciiTraceHelper h_as;
    //Ptr<OutputStreamWrapper> routes = h_as.CreateFileStream("mptcp.routes");
    //ueStaticRouting->PrintRoutingTable(routes);

    gwStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.10.1.2"), 2);//?
    gwStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.10.1.2"), 3);
    
    apStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.10.2.2"), 2);
    
    //routerStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.20.1.0"), Ipv4Mask ("255.255.255.0"), 3);
    routerStaticRouting->AddHostRouteTo (Ipv4Address ("7.0.0.2"), Ipv4Address ("10.10.1.1"), 1);
    routerStaticRouting->AddHostRouteTo (Ipv4Address ("10.1.2.1"), Ipv4Address ("10.10.2.1"), 2);
    
    //remote to UE
    remoteStaticRouting->AddHostRouteTo (Ipv4Address ("10.1.2.1"), Ipv4Address ("10.20.1.1"), 1);
    remoteStaticRouting->AddHostRouteTo (Ipv4Address ("7.0.0.2"), Ipv4Address ("10.20.1.1"), 1);
    
    /* Generate Route. */

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
    PrintNodeInfo(gwLte);
    PrintNodeInfo(hostRoutersContainer);
    PrintNodeInfo(hostRemotesContainer);

    NS_LOG_INFO ("Create Applications.");

    MpTcpPacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    //sink.SetAttribute("SendSize", UintegerValue(100000));
    ApplicationContainer sinkApps = sink.Install(hostRemote);
    sinkApps.Start(Seconds(startTime));
    sinkApps.Stop(Seconds(stopTime-1));

    Ipv4Address dstaddr ("10.20.1.2");
    
    MpTcpBulkSendHelper sourceHelper("ns3::TcpSocketFactory", InetSocketAddress(dstaddr, port));
    sourceHelper.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApp = sourceHelper.Install(ueNode);
    sourceApp.Start(Seconds(startTime));
    sourceApp.Stop(Seconds(stopTime-1));

    AnimationInterface anim ("simple.xml");
   
    //AnimationInterface::SetNodeColor(ueNode, 0, 0, 205);
    anim.UpdateNodeColor(ueNode, 0, 0, 205);
    anim.UpdateNodeDescription(ueNode, "ueNode");
    anim.UpdateNodeColor(hostRemote, 0, 100, 100);
    anim.UpdateNodeDescription(hostRemote, "sinkNode");
    anim.UpdateNodeDescription(apNode, "apWifi");
    anim.UpdateNodeDescription(enbNode, "enbNode");
    anim.UpdateNodeDescription(hostRouter, "Router");
    anim.UpdateNodeDescription(gwLte, "PGW");
   
    AsciiTraceHelper h_ascii;
    /* Flow Monitor Configuration */
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Gnuplot2dDataset dat1,dat2;
    //Simulator::Schedule(Seconds(0.1),&ThroughputMonitor, monitor, monitor, dat1, dat2);
    ThroughputMonitor(&flowmon, monitor, dat1,dat2);
    
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (Seconds(stopTime));
    Simulator::Run();
    
    monitor->CheckForLostPackets();
    
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter;
    Ptr<OutputStreamWrapper> output = h_ascii.CreateFileStream("mptcp.Results");
    for (iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        if ((t.sourceAddress == "7.0.0.2" && t.destinationAddress == "10.20.1.2"))
        {
            printIterator(iter, output, t);
        }
        if ((t.sourceAddress == "10.1.2.1" && t.destinationAddress == "10.20.1.2"))
        {
            printIterator(iter, output, t);
        }
        else{
            //printIterator(iter, output, t);
        }
    }
    //monitor->SerializeToXmlFile("mptcp.flowmon", false, false);
    
    
    
    if (isPcapEnabled)
    {
        p2pGw2router.EnablePcapAll ("mptcp-lte.pcap",false);
        p2pAp2router.EnablePcapAll ("mptcp-wifi.pcap", false);
        //lteHelper->EnableTraces();
        
    }
    
    PlotThroughput(dat1,dat2);
    
    Simulator::Destroy();
    NS_LOG_INFO ("Done.");

}
