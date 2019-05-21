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
 *                                                                                           bottleneck
 *             0 <--------------------------> 0.eNB.2 <-----------------------> 0.
 *           /        10 Mb/s, 10 ms    droptail/codel       100 Mb/s, 5ms       \
 *     source                                                                   Router.2<queue>-----1.sink
 *           \                                                                   /
 *            1 <--------------------------> 0.WiFi.1 <------------------------>1.
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


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CodelDropTailMpTcpLteWifi");


void ReceivePacket (Ptr<Socket> socket);
static void SendPacket (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval );

double lastrx_a = 0;
double lastrx_b = 0;
double lastrx_c = 0;

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, std::string filename_f1,std::string filename_f2)
{
    double localThrou=0;
    double firstTX = 0.0;
    double lastRX = 0.0;

    std::string tcpflow = "throughput.udp.tr";

    Ptr<OutputStreamWrapper> stream1 = Create<OutputStreamWrapper>(filename_f1.c_str(), std::ios::app);
    Ptr<OutputStreamWrapper> stream2 = Create<OutputStreamWrapper>(filename_f2.c_str(), std::ios::app);
    Ptr<OutputStreamWrapper> stream3 = Create<OutputStreamWrapper>(tcpflow.c_str(), std::ios::app);

    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
        Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
        uint32_t bytesRX = stats->second.rxBytes;
        double last_s = stats->second.timeLastRxPacket.GetSeconds();
        localThrou = (stats->second.rxBytes * 8.0)/(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstRxPacket.GetSeconds())/(1024*1024);

        if (firstTX > stats->second.timeFirstTxPacket.GetSeconds()) {
            firstTX = stats->second.timeFirstTxPacket.GetSeconds();
        }
        if (lastRX < stats->second.timeLastRxPacket.GetSeconds()) {
            lastRX = stats->second.timeLastRxPacket.GetSeconds();
        }

        if ((fiveTuple.sourceAddress == "7.0.0.2" && fiveTuple.destinationAddress == "10.20.1.2"))
        {

            if(last_s > lastrx_a){
                lastrx_a = last_s;
                //if ((double)localThrou >= 0)
                *stream1->GetStream () << (double)Simulator::Now().GetSeconds() << " " << (double)localThrou << " " << std::endl;
            }
        }
        if ((fiveTuple.sourceAddress == "10.1.2.1" && fiveTuple.destinationAddress == "10.20.1.2"))
        {
            if(last_s > lastrx_b){
                lastrx_b = last_s;
                //if ((double)localThrou >= 0)
                    *stream2->GetStream () << (double)Simulator::Now().GetSeconds() << " " << (double)localThrou << " " << std::endl;
            }
        }
        if ((fiveTuple.sourceAddress == "10.50.1.1" && fiveTuple.destinationAddress == "10.20.1.2"))
        {
            if(last_s > lastrx_c){
                lastrx_c = last_s;
                //if ((double)localThrou >= 0)
                    *stream3->GetStream () << (double)Simulator::Now().GetSeconds() << " " << (double)localThrou << " " << std::endl;
            }
        }

    }
    Simulator::Schedule(Seconds(0.5),&ThroughputMonitor, fmhelper, flowMon, filename_f1, filename_f2);
}

void GoodputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, ApplicationContainer app, std::string filename, float addDelay)
{
     Ptr<MpTcpPacketSink> sinks = DynamicCast<MpTcpPacketSink> (app.Get (0));
    Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(filename.c_str(), std::ios::app);

    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());

    uint32_t bytesRX = 0;

    bytesRX = bytesRX + sinks->GetTotalRx();

    double firstTX = 0.0;//600
    double lastRX = 0.0;
    double goodput_a = 0.0;
    double goodput_b = 0.0;
    double goodput_c = 0.0;
    bool log = false;

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
        Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);

        if (firstTX > stats->second.timeFirstTxPacket.GetSeconds()) {
            firstTX = stats->second.timeFirstTxPacket.GetSeconds();
        }
        if (lastRX < stats->second.timeLastRxPacket.GetSeconds()) {
            lastRX = stats->second.timeLastRxPacket.GetSeconds();
        }

        //goodput = (bytesRX*8)/(lastRX-firstTX)/(1024*1024);

        if ((fiveTuple.sourceAddress == "7.0.0.2" && fiveTuple.destinationAddress == "10.20.1.2"))
        {
            goodput_a = (stats->second.rxBytes * 8.0)/(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/(1024*1024);
        }
        if ((fiveTuple.sourceAddress == "10.1.2.1" && fiveTuple.destinationAddress == "10.20.1.2"))
        {
            goodput_b = (stats->second.rxBytes * 8.0)/(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/(1024*1024);
        }
        if ((fiveTuple.sourceAddress == "10.50.1.1" && fiveTuple.destinationAddress == "10.20.1.2"))
        {
            goodput_c = (stats->second.rxBytes * 8.0)/(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/(1024*1024);
        }
    }
    *stream->GetStream ()  << " " <<  goodput_a << " " << goodput_b << " " << goodput_c << std::endl;
   // NS_LOG_UNCOND("Sink Bytes " << " " << (goodput_a+goodput_b+goodput_c) << " " <<  goodput_a << " " << goodput_b << " " << goodput_c << " Rx " << bytesRX);

    Simulator::Schedule(Seconds(1), &GoodputMonitor, fmhelper, flowMon, app, filename, addDelay);

}

static void
SojournTracer (Ptr<OutputStreamWrapper>stream, Time oldval, Time newval)
{
    *stream->GetStream () << oldval << " " << newval << " " << Simulator::Now ().GetSeconds () << std::endl;
}

static void
TraceSojourn (std::string sojournTrFileName, std::string queueType)
{
    AsciiTraceHelper ascii;
    if (sojournTrFileName.compare ("") == 0)
    {
        NS_LOG_DEBUG ("No trace file for sojourn provided");
        return;
    }
    else
    {
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (sojournTrFileName.c_str ());
        Config::ConnectWithoutContext ("/NodeList/2/DeviceList/4/$ns3::PointToPointNetDevice/TxQueue/$ns3::"+queueType+"/Sojourn", MakeBoundCallback (&SojournTracer, stream));
    }
}

static void
QueueLengthTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
}

static void
TraceQueueLength (std::string queueLengthTrFileName, std::string queueType)
{
    AsciiTraceHelper ascii;
    if (queueLengthTrFileName.compare ("") == 0)
    {
        NS_LOG_DEBUG ("No trace file for queue length provided");
        return;
    }
    else
    {
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (queueLengthTrFileName.c_str ());
        Config::ConnectWithoutContext ("/NodeList/2/DeviceList/4/$ns3::PointToPointNetDevice/TxQueue/$ns3::"+queueType+"/QueueStackSize", MakeBoundCallback (&QueueLengthTracer, stream));
    }
}

static void
EveryDropTracer (Ptr<OutputStreamWrapper>stream, Ptr<const Packet> p)
{
  //  SeqTsHeader seqTs;
  //  p->PeekHeader (seqTs);
    //(double)seqTs.GetSeq()
    Ptr<Packet> pp = p->Copy();
    Ipv4Header ipv4;
    // removi no ipv4_header.cc a linha do NS_ASSERT ... voltar depois
    pp->RemoveHeader(ipv4);
    *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << ipv4.GetSource() << std::endl;
}

static void
TraceEveryDrop (std::string everyDropTrFileName)
{
    AsciiTraceHelper ascii;
    if (everyDropTrFileName.compare ("") == 0)
    {
        NS_LOG_UNCOND ("No trace file for every drop event provided");
        return;
    }
    else
    {
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (everyDropTrFileName.c_str ());
        Config::ConnectWithoutContext ("/NodeList/2/DeviceList/4/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeBoundCallback (&EveryDropTracer, stream));
    }
}

static void
DropCountTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
}

static void
TraceDropsCount (std::string queueLengthTrFileName, std::string queueType)
{
    AsciiTraceHelper ascii;
    if (queueLengthTrFileName.compare ("") == 0)
    {
        NS_LOG_DEBUG ("No trace file for queue length provided");
        return;
    }
    else
    {
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (queueLengthTrFileName.c_str ());
        Config::ConnectWithoutContext ("/NodeList/2/DeviceList/4/$ns3::PointToPointNetDevice/TxQueue/$ns3::"+queueType+"/DropsCount", MakeBoundCallback (&DropCountTracer, stream));
    }
}

static void
DroppingStateTracer (Ptr<OutputStreamWrapper>stream, bool oldVal, bool newVal)
{
    if (oldVal == false && newVal == true)
    {
        NS_LOG_INFO ("Entering the dropping state");
        *stream->GetStream () << Simulator::Now ().GetSeconds () << " ";
    }
    else if (oldVal == true && newVal == false)
    {
        NS_LOG_INFO ("Leaving the dropping state");
        *stream->GetStream () << Simulator::Now ().GetSeconds ()  << std::endl;
    }
}

static void
TraceDroppingState (std::string dropStateTrFileName, std::string queueType)
{
    AsciiTraceHelper ascii;
    if (dropStateTrFileName.compare ("") == 0)
    {
        NS_LOG_DEBUG ("No trace file for dropping state provided");
        return;
    }
    else
    {
        Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (dropStateTrFileName.c_str ());
        Config::ConnectWithoutContext ("/NodeList/2/DeviceList/4/$ns3::PointToPointNetDevice/TxQueue/$ns3::"+queueType+"/DropState", MakeBoundCallback (&DroppingStateTracer, stream));
    }
}

int
main(int argc, char *argv[]){


    uint16_t port = 8;
    uint32_t queueSize = 100;
    uint32_t repRun = 1;
    uint32_t bufferSize = 65535;//64kb
    std::string routerWanQueueType = "CoDelQueueLifo";      //DropTail, CoDelQueue, CoDelQueueLifo
    std::string linkType = "wifi";       //wifi or mmwave
    uint32_t pktSize = 1458; //in bytes. 1458 to prevent fragments
    uint32_t CWND = 65535;//60pkt * x segments
    std::string dataRate = "1";
    std::string congestionControl = "RTT_Compensator";

    float delayRate = 0.100;// range de 0 a 80

    float startTime = 0.1;
    float simDuration = 120;      //in seconds

    bool isPcapEnabled = true;
    bool logging = false;
    bool interference = true;

    std::string CoDelInterval = "100ms";
    std::string CoDelTarget = "5ms";
    std::string fileNamePrefix = "codel-vs-droptail-bfs";

    uint32_t path2use = 0;


    CommandLine cmd;
    cmd.AddValue ("bufferSize", "Snd/Rcv buffer size.", bufferSize);
    cmd.AddValue ("queueSize", "Queue size in packets", queueSize);
    cmd.AddValue ("dataRate", "rate in Mb/s for fast path", dataRate);
    cmd.AddValue ("delayRate", "rate delay", delayRate);
    cmd.AddValue ("queueType", "Queue type: DropTail, CoDel", routerWanQueueType);
    cmd.AddValue ("logging", "Flag to enable/disable logging", logging);
    cmd.AddValue ("pcap", "Flag to enable/disable pcap", isPcapEnabled);
    cmd.AddValue ("fileNamePrefix", "file of save results", fileNamePrefix);
    cmd.AddValue ("congestionControl", "congestion control of MPTCP", congestionControl);
    cmd.AddValue ("simTime", "time of simulation", simDuration);
    cmd.AddValue ("repRun", "number of simulation repeat", repRun);
    cmd.AddValue ("interference", "enable interference app", interference);
    cmd.AddValue ("path2use", "enable path that it use", path2use);
    cmd.Parse (argc, argv);


    std::string fileGoodput = fileNamePrefix + "-goodput" + ".xr";

    float stopTime = startTime + simDuration;
    std::stringstream ss;
    ss << dataRate << "Mbps";
    dataRate = ss.str();

    std::string pcapFileName = fileNamePrefix + "-" + routerWanQueueType;
    std::string attributeFileName = fileNamePrefix + "-" + ".attr";
    std::string sojournTrFileName = fileNamePrefix + "-sojourn" + ".tr";
    std::string queueLengthTrFileName = fileNamePrefix + "-length" + ".tr";
    std::string everyDropTrFileName = fileNamePrefix + "-drop" + ".tr";
    std::string dropStateTrFileName = fileNamePrefix + "-drop-state" + ".tr";

    std::string fileThroughput0 = fileNamePrefix + "-throughput-0" + ".tr";
    std::string fileThroughput1 = fileNamePrefix + "-throughput-1" + ".tr";

    //std::cout << "++++++++++++++ Rate:" << dataRate << " Delay:" << delayRate << std::endl;

    if (logging)
    {
        //LogComponentEnable ("CodelDropTailMpTcpLteWifi", LOG_LEVEL_ALL);
        //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
        //LogComponentEnable("MpTcpSocketBase", LOG_INFO);
        //LogComponentEnable("TcpHeader", LOG_ALL);
        //LogComponentEnable("Packet", LOG_ALL);
        //LogComponentEnable("TcpL4Protocol", LOG_ALL);
    }

    // Enable checksum
    if (isPcapEnabled)
    {
        //GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    }
    //SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
    SeedManager::SetRun (repRun);

    Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
    //Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktSize));
    //Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(MpTcpSocketBase::GetTypeId()));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue(CWND));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(bufferSize));//send buffer of 64Kb
    //Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(4096));//send buffer of 64Kb - inserido 16-11


    Config::SetDefault("ns3::MpTcpSocketBase::MaxSubflows", UintegerValue(8));
    /* Uncoupled_TCPs, Linked_Increases, RTT_Compensator, Fully_Coupled */
    Config::SetDefault("ns3::MpTcpSocketBase::CongestionControl", StringValue(congestionControl));
    //Config::SetDefault("ns3::MpTcpSocketBase::PathManagement", StringValue("FullMesh"));
    //Config::SetDefault("ns3::MpTcpSocketBase::LargePlotting", BooleanValue(true));
    Config::SetDefault("ns3::MpTcpSocketBase::ShortPlotting", BooleanValue(true));
    Config::SetDefault("ns3::MpTcpSocketBase::ShortFlowTCP", BooleanValue(true));
    Config::SetDefault("ns3::MpTcpBulkSendApplication::FlowType", StringValue("Short"));

    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    //Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("DsssRate1Mbps"));

    // Configure the queue
    if (routerWanQueueType.compare ("DropTail") == 0)
    {
        //LogComponentEnable ("DropTailQueue", LOG_LEVEL_ALL);
        Config::SetDefault("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
        Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(queueSize));

    }
    else if (routerWanQueueType.compare ("CoDelQueueLifo") == 0)
    {
        //LogComponentEnable ("CoDelQueue", LOG_LEVEL_ALL);
        //LogComponentEnable ("CoDelQueue", LOG_LEVEL_FUNCTION);
        Config::SetDefault("ns3::CoDelQueueLifo::Mode", StringValue("Packets"));
        Config::SetDefault("ns3::CoDelQueueLifo::MaxPackets", UintegerValue(queueSize));
        Config::SetDefault ("ns3::CoDelQueueLifo::Interval", StringValue(CoDelInterval));
        Config::SetDefault ("ns3::CoDelQueueLifo::Target", StringValue(CoDelTarget));
    }
    else if (routerWanQueueType.compare ("CoDelQueue") == 0)
    {
        //LogComponentEnable ("CoDelQueue", LOG_LEVEL_ALL);
        //LogComponentEnable ("CoDelQueue", LOG_LEVEL_FUNCTION);
        Config::SetDefault("ns3::CoDelQueue::Mode", StringValue("Packets"));
        Config::SetDefault("ns3::CoDelQueue::MaxPackets", UintegerValue(queueSize));
        Config::SetDefault ("ns3::CoDelQueue::Interval", StringValue(CoDelInterval));
        Config::SetDefault ("ns3::CoDelQueue::Target", StringValue(CoDelTarget));
    }
    else if (routerWanQueueType.compare ("Fq_CoDel") == 0)
    {
        Config::SetDefault ("ns3::Fq_CoDelQueue::headMode", BooleanValue (true));
        Config::SetDefault ("ns3::Fq_CoDelQueue::peturbInterval", UintegerValue(131));
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

    // TCP Node
    Ptr<Node> sourceNode = CreateObject<Node> ();
    Names::Add ("sourceTcp", sourceNode);
    NodeContainer sourceContainer = NodeContainer(sourceNode);

    Ptr<Node> hostRouter = CreateObject<Node> ();
    Names::Add ("router", hostRouter);
    NodeContainer hostRoutersContainer = NodeContainer(hostRouter);

    InternetStackHelper internet;
    internet.Install (hostRemote);
    internet.Install(hostRouter);
    internet.Install(sourceNode);//TCP node

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
    positionAlloc->Add (Vector(20, 20, 0));//TCP node
    positionAlloc->Add (Vector(10, 25, 0));
    positionAlloc->Add (Vector(10, 10, 0));

    MobilityHelper mobilityEnb;
    mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityEnb.SetPositionAllocator(positionAlloc);

    // -------------------------------------------------------------------------------
    //  WIFI NETWORK CONFIGURATION
    // -------------------------------------------------------------------------------
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
    //wifi.SetStandard (WIFI_PHY_STANDARD_80211a);

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
    //Pathloss Models
    //lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisPropagationLossModel"));
    //lteHelper->SetPathlossModelType("ns3::FriisPropagationLossModel");

    Ptr<Node> gwLte = epcHelper->GetPgwNode ();
    Names::Add ("pGw", gwLte);

    mobilityEnb.Install(ueNodes);
    mobilityEnb.Install(enbNodes);
    mobilityEnb.Install(gwLte);
    mobilityEnb.Install(hostRemotesContainer);
    mobilityEnb.Install(sourceContainer);//TCP node
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
    NodeContainer nSource2router = NodeContainer(sourceNode,hostRouter);//TCP node

    PointToPointHelper p2p;

    //p2p for pgw to router
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
    p2p.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (delayRate)));   //0.010
    NetDeviceContainer dGw2router = p2p.Install (nGw2router);
    Names::Add ("gw/ifrouter", dGw2router.Get (0));
    Names::Add ("router/ifgw", dGw2router.Get (1));

    //p2p for AP to roter
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
    p2p.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (0.001)));   //0.010
    NetDeviceContainer dAp2router = p2p.Install (nAp2router);

    //Source to roter
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
    p2p.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (0.001)));   //0.010
    NetDeviceContainer dSource2router = p2p.Install (nSource2router); //TCP node

    //p2p router to sink
    p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
    p2p.SetDeviceAttribute ("Mtu", UintegerValue (pktSize));
    p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (0.005)));
    NetDeviceContainer dRouter2sink = p2p.Install (nRouter2sink);

    Ptr<PointToPointNetDevice> router2HostDev = DynamicCast<PointToPointNetDevice> (dRouter2sink.Get (0));
    router2HostDev->SetAttribute ("DataRate", StringValue (dataRate));

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

    // source to router
    ipv4h.SetBase("10.50.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifSourcer2router = ipv4h.Assign (dSource2router);

    // -------------------------------------------------------------------------------
    //  QUEUE CONFIGURATION ::: ROUTER<queue> ---- SINK
    // -------------------------------------------------------------------------------

    // Configure the queue
    if (routerWanQueueType.compare ("DropTail") == 0)
    {
        Ptr<Queue> dropTail = CreateObject<DropTailQueue> ();
        router2HostDev->SetQueue (dropTail);
    }
    else if (routerWanQueueType.compare ("CoDelQueueLifo") == 0)
    {
        Ptr<Queue> codel = CreateObject<CoDelQueueLifo> ();
        router2HostDev->SetQueue (codel);
    }
    else if (routerWanQueueType.compare ("CoDelQueue") == 0)
    {
        Ptr<Queue> codel = CreateObject<CoDelQueue> ();
        router2HostDev->SetQueue (codel);
    }
    else if (routerWanQueueType.compare ("Fq_CoDel") == 0)
    {
        Ptr<Queue> sfq = CreateObject<Fq_CoDelQueue> ();
        router2HostDev->SetQueue (sfq);
    }
    else
    {
        NS_LOG_DEBUG ("Invalid queue type");
        exit (1);
    }

    // -------------------------------------------------------------------------------
    //  STATIC ROUTING CONFIGURATION
    // -------------------------------------------------------------------------------

    Ptr<Ipv4> ipv4Ue = ueNode->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Router = hostRouter->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Remote = hostRemote->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4pgw = gwLte->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4SAp = apNode->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Source = sourceNode->GetObject<Ipv4> ();

    Ipv4StaticRoutingHelper ipv4RoutingHelper;

    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Ue);
    Ptr<Ipv4StaticRouting> gwStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4pgw);
    Ptr<Ipv4StaticRouting> apStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4SAp);
    Ptr<Ipv4StaticRouting> routerStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Router);
    Ptr<Ipv4StaticRouting> remoteStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Remote);
    Ptr<Ipv4StaticRouting> sourceStaticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Source);

    //routerStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.20.1.0"), Ipv4Mask ("255.255.255.0"), 2);
    if(path2use == 0){
      ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.1.2.2"), 1,2);
      ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("7.0.0.1"), 2,2);
    }
    if(path2use == 1){
      ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.1.2.2"), 1,2);
      //ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("7.0.0.1"), 2,2);
    }
    if(path2use == 2){
      //ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.1.2.2"), 1,2);
      ueStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("7.0.0.1"), 2,2);
    }



    gwStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.10.1.2"), 2);//?
    gwStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.10.1.2"), 3);

    apStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.10.2.2"), 2);

    //routerStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.20.1.0"), Ipv4Mask ("255.255.255.0"), 3);
    routerStaticRouting->AddHostRouteTo (Ipv4Address ("7.0.0.2"), Ipv4Address ("10.10.1.1"), 1);
    routerStaticRouting->AddHostRouteTo (Ipv4Address ("10.1.2.1"), Ipv4Address ("10.10.2.1"), 2);

    //remote to UE
    remoteStaticRouting->AddHostRouteTo (Ipv4Address ("10.1.2.1"), Ipv4Address ("10.20.1.1"), 1);
    remoteStaticRouting->AddHostRouteTo (Ipv4Address ("7.0.0.2"), Ipv4Address ("10.20.1.1"), 1);

    //source to remote
    //routerStaticRouting->AddHostRouteTo (Ipv4Address ("10.40.1.2"), Ipv4Address ("10.40.1.2"), 4);
    routerStaticRouting->AddHostRouteTo (Ipv4Address ("10.50.1.1"), Ipv4Address ("10.50.1.1"), 5);

    sourceStaticRouting->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.50.1.2"), 1);
    //udpStaticRouting->AddHostRouteTo (Ipv4Address ("10.50.1.1"), Ipv4Address ("10.40.1.1"), 1);

    // -------------------------------------------------------------------------------
    //  APP CONFIGURATION
    // -------------------------------------------------------------------------------

    NS_LOG_INFO ("Create Applications.");

    MpTcpPacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    //sink.SetAttribute("SendSize", UintegerValue(100000));
    ApplicationContainer sinkApps = sink.Install(hostRemote);
    sinkApps.Start(Seconds(startTime));
    sinkApps.Stop(Seconds(stopTime-1));

    Ipv4Address dstaddr ("10.20.1.2");

    MpTcpBulkSendHelper sourceHelper("ns3::TcpSocketFactory", InetSocketAddress(dstaddr, port));
    sourceHelper.SetAttribute("MaxBytes", UintegerValue(4194304));//UintegerValue(4194304)4mbytes
    ApplicationContainer sourceApp = sourceHelper.Install(ueNode);
    sourceApp.Start(Seconds(startTime));
    sourceApp.Stop(Seconds(stopTime-1));

    // -----------------------
    // APP 2
    // -----------------------

    NS_LOG_INFO ("Create sockets.");
    //Receiver socket on n1
    //TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    Ptr<Socket> recvSink = Socket::CreateSocket (hostRemote, tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 4477);
    recvSink->Bind (local);
    recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
    Ipv4Address udpdstaddr ("10.20.1.2");

    //Sender socket on n0
    Ptr<Socket> source = Socket::CreateSocket (sourceNode, tid);
    InetSocketAddress remote = InetSocketAddress (udpdstaddr, 4477);
    source->Connect (remote);

    //Schedule SendPacket
    uint32_t packetSize = 1024;
    uint32_t packetCount = 1000000;
    double packetInterval = 0.04;

    Time interPacketInterval = Seconds (packetInterval);

    if(interference){

    Simulator::ScheduleWithContext (source->GetNode ()->GetId (), Seconds (0.01), &SendPacket, source, packetSize, packetCount, interPacketInterval);
    }
    AnimationInterface anim ("globecom-one-receiver.xml");

    uint32_t size = 1;
    anim.UpdateNodeColor(ueNode, 0, 0, 205);
    anim.UpdateNodeDescription(ueNode, "ueNode");
    anim.UpdateNodeSize(ueNode->GetId(), size, size);

    anim.UpdateNodeColor(hostRemote, 0, 100, 100);
    anim.UpdateNodeDescription(hostRemote, "sinkNode");
    anim.UpdateNodeSize(hostRemote->GetId(), size, size);

    anim.UpdateNodeColor(sourceNode, 100, 100, 100);
    anim.UpdateNodeDescription(sourceNode, "sourceNode");
    anim.UpdateNodeSize(sourceNode->GetId(), size, size);

    anim.UpdateNodeDescription(apNode, "apWifi");
    anim.UpdateNodeSize(apNode->GetId(), size, size);

    anim.UpdateNodeDescription(enbNode, "enbNode");
    anim.UpdateNodeSize(enbNode->GetId(), size, size);

    anim.UpdateNodeDescription(hostRouter, "Router");
    anim.UpdateNodeSize(hostRouter->GetId(), size, size);

    anim.UpdateNodeDescription(gwLte, "PGW");
    anim.UpdateNodeSize(gwLte->GetId(), size, size);

    AsciiTraceHelper h_ascii;
    /* Flow Monitor Configuration */
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Ptr<MpTcpPacketSink> sink1 = DynamicCast<MpTcpPacketSink> (sinkApps.Get (0));

    monitor->CheckForLostPackets();
    ThroughputMonitor(&flowmon, monitor, fileThroughput0,fileThroughput1);

    GoodputMonitor(&flowmon, monitor,sinkApps,fileGoodput, delayRate);

    NS_LOG_INFO ("Run Simulation.");

    if (isPcapEnabled)
    {

        if (routerWanQueueType.compare ("DropTail") == 0)
        {
            TraceEveryDrop (everyDropTrFileName);
        }
        if (routerWanQueueType.compare ("CoDelQueue") == 0)
        {
            TraceEveryDrop (everyDropTrFileName);
            TraceSojourn (sojournTrFileName,routerWanQueueType);
            TraceQueueLength (queueLengthTrFileName,routerWanQueueType);
            TraceDroppingState (dropStateTrFileName,routerWanQueueType);
        }
        if (routerWanQueueType.compare ("CoDelQueueLifo") == 0)
        {
            TraceDropsCount (everyDropTrFileName,routerWanQueueType);
            TraceSojourn (sojournTrFileName,routerWanQueueType);
            TraceQueueLength (queueLengthTrFileName,routerWanQueueType);
            TraceDroppingState (dropStateTrFileName,routerWanQueueType);
        }

        if (routerWanQueueType.compare ("Fq_CoDel") == 0)
        {
            routerWanQueueType = "CoDelQueueLifo";
            TraceEveryDrop (everyDropTrFileName);
            TraceSojourn (sojournTrFileName,routerWanQueueType);
            TraceQueueLength (queueLengthTrFileName,routerWanQueueType);
            TraceDroppingState (dropStateTrFileName,routerWanQueueType);
        }

    }

    Simulator::Stop (Seconds(stopTime));
    Simulator::Run();

    Simulator::Destroy();
    NS_LOG_INFO ("Done.");

}

void ReceivePacket (Ptr<Socket> socket)
{
    NS_LOG_INFO ("Received one packet!");
    Ptr<Packet> packet = socket->Recv ();
}

static void SendPacket (Ptr<Socket> socket, uint32_t pktSize,
                        uint32_t pktCount, Time pktInterval )
{
    if (pktCount > 0)
    {
        socket->Send (Create<Packet> (pktSize));
        Simulator::Schedule (pktInterval, &SendPacket,
                             socket, pktSize,pktCount - 1, pktInterval);
    }
    else
    {
        socket->Close ();
    }
}
