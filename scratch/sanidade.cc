#define NS_LOG_APPEND_CONTEXT \
{ std::clog << Simulator::Now ().GetSeconds ()<< "  ";}

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/config-store.h"
#include "ns3/file-config.h"
#include "ns3/gtk-config-store.h"
#include "ns3/netanim-module.h"
#include "ns3/mp-tcp-bulk-send-helper.h"
#include "ns3/mp-tcp-packet-sink-helper.h"

#include "ns3/flow-classifier.h"
#include "ns3/flow-monitor-module.h"

#include "ns3/ipv4-header.h"

#include "ns3/gnuplot-helper.h"

//         10.0.0.0
//       ----r2------
//    s               r3 ---- D
//       ----r1------
//         10.0.1.0

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("MpTcpNewReno");

Ptr<Node> nodeS;
Ptr<Node> nodeS2;
Ptr<Node> nodeD;
Ptr<Node> routerR1;
Ptr<Node> routerR2;
Ptr<Node> routerR3;

void
PrintNodeInfo(NodeContainer nodes){

    Ptr< Node > node_p;
    Ptr< NetDevice > device_p;

    Ptr<Ipv4> ipv4;
    Ipv4InterfaceAddress iaddr;
    Ipv4Address m_ipAddr;
    Address m_addr;

    NS_LOG_UNCOND ("PrintNodeList");
    for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
        node_p = nodes.Get (i);
        std::string nodeName = Names::FindName (node_p);
        if(Names::FindName (node_p)!= "")
            NS_LOG_UNCOND ("NodeId: " << node_p->GetId () << " Name: " << nodeName);
        else
            NS_LOG_UNCOND ("NodeId: " << node_p->GetId () );
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
            NS_LOG_UNCOND ("  Device " << j << " IP " << m_ipAddr << " MAC " << m_addr);
        }
    }

}



void GoodputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Ptr<MpTcpPacketSink> sinks, std::string filename, uint64_t delta)
{
    double goodput = 0.0;

    Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(filename.c_str(), std::ios::app);

    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());

    uint32_t bytesRX = 0;

    bytesRX = sinks->GetTotalRx();

    //NS_LOG_UNCOND(Simulator::Now ().GetSeconds () << " " << bytesRX);

    double firstTX = 0.0;//600
    double lastRX = 0.0;
    double goodput_a = 0.0;
    double goodput_b = 0.0;
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

        if ((fiveTuple.sourceAddress == "10.0.0.1" && fiveTuple.destinationAddress == "10.0.4.2"))
        {
            goodput_b = (stats->second.rxBytes * 8.0)/(stats->second.timeLastTxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/(1024*1024);
            *stream->GetStream () << fiveTuple.sourceAddress << " " << goodput_b << " " << stats->second.txPackets << " ";
             if(log){
                NS_LOG_UNCOND("Src Addr " << fiveTuple.sourceAddress);
                NS_LOG_UNCOND("Tx Bytes = " << stats->second.txPackets);
                NS_LOG_UNCOND("Rx Bytes = " << stats->second.rxPackets);
                NS_LOG_UNCOND("Goodput = " << goodput_a);
                NS_LOG_UNCOND("Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
            }


        }
        if ((fiveTuple.sourceAddress == "10.0.1.1" && fiveTuple.destinationAddress == "10.0.4.2"))
        {
            goodput_a = (stats->second.rxBytes * 8.0)/(stats->second.timeLastTxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/(1024*1024);
            *stream->GetStream () << fiveTuple.sourceAddress << " " << goodput_b << " " << stats->second.txPackets << " ";
            if(log){
                NS_LOG_UNCOND("Src Addr " << fiveTuple.sourceAddress);
                NS_LOG_UNCOND("Tx Bytes = " << stats->second.txPackets);
                NS_LOG_UNCOND("Rx Bytes = " << stats->second.rxPackets);
                NS_LOG_UNCOND("Goodput = " << goodput_b);
                NS_LOG_UNCOND("Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
            }


        }

    }
      *stream->GetStream () << std::endl;
      Simulator::Schedule(Seconds(1),&GoodputMonitor, fmhelper, flowMon, sinks, filename, delta);
}
//mudei no ipv4-global-routeing .... adicionei case 1 linha 181

int
main(int argc, char *argv[])
{
    std::string filename = "trace_";
    std::string dataRate = "1000";//0.5,1,5,10
    uint64_t delta = 40;// range de 0 a 80
    uint64_t delay = 40;//100,200,300
    uint64_t packetSize = 1480;
    uint32_t queueSize = 100;
    uint32_t CWND = 65535;//pkt*34s
    uint32_t bufferSize = 65536;//64kb
    uint32_t simTime = 60;

    CommandLine cmd;
    cmd.AddValue ("dataRate", "Data Rate for flow, 0.5,1,5,10 Mb/s", dataRate);
    cmd.AddValue ("delta", "delta value, range of 0...80", delta);
    cmd.AddValue ("delay", "delay, with 100,200 or 300", delay);
    cmd.AddValue ("filename", "file of save results", filename);
    cmd.AddValue ("simTime", "simulation time", simTime);
    cmd.Parse (argc, argv);

    uint32_t startTime = 0;
    uint32_t endTime = simTime-startTime;

    /* Uncoupled_TCPs, Linked_Increases, RTT_Compensator, Fully_Coupled */
    Config::SetDefault("ns3::MpTcpSocketBase::CongestionControl", StringValue("Fully_Coupled"));
    Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(MpTcpSocketBase::GetTypeId()));
    Config::SetDefault("ns3::MpTcpSocketBase::MaxSubflows", UintegerValue(8)); // Sink

    Config::SetDefault("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(queueSize));

    //Config::SetDefault("ns3::DropTailQueue::MaxPackets",UintegerValue(queueSize));
    //Config::SetDefault("ns3::DropTailQueue::Mode", EnumValue(DropTailQueue::QUEUE_MODE_PACKETS));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue(CWND));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(bufferSize));//send buffer of 64Kb

    std::stringstream ss;
    ss << dataRate << "Mb/s";
    dataRate = ss.str();

    //LogComponentEnable("MpTcpNewReno", LOG_LEVEL_ALL);
    //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
    //LogComponentEnable("MpTcpSocketBase", LOG_INFO);

    /* Build nodes. */
    NodeContainer s;
    s.Create(1);

    NodeContainer s2;
    s2.Create(1);

    NodeContainer d;
    d.Create(1);

    NodeContainer r1;
    r1.Create(1);
    NodeContainer r2;
    r2.Create(1);
    NodeContainer r3;
    r3.Create(1);


    /* Build link. */
    PointToPointHelper p2p_sTor1;
    p2p_sTor1.SetDeviceAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    p2p_sTor1.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay+delta)));

    PointToPointHelper p2p_s2Tor1;
    p2p_s2Tor1.SetDeviceAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    p2p_s2Tor1.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay+delta)));

    PointToPointHelper p2p_sTor2;
    p2p_sTor2.SetDeviceAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    p2p_sTor2.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));

    PointToPointHelper p2p_r1Tor3;
    p2p_r1Tor3.SetDeviceAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    p2p_r1Tor3.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay+delta)));

    PointToPointHelper p2p_r2Tor3;
    p2p_r2Tor3.SetDeviceAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    p2p_r2Tor3.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));

    PointToPointHelper p2p_r3Tod;
    p2p_r3Tod.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Mb/s")));
    p2p_r3Tod.SetChannelAttribute("Delay", TimeValue(MilliSeconds(5)));


    /* Build link net device container. */
    NodeContainer nc_s_r1;
    nc_s_r1.Add(s);
    nc_s_r1.Add(r1);
    NetDeviceContainer dev_s_r1 = p2p_sTor1.Install(nc_s_r1);

    NodeContainer nc_s_r2;
    nc_s_r2.Add(s);
    nc_s_r2.Add(r2);
    NetDeviceContainer dev_s_r2 = p2p_sTor2.Install(nc_s_r2);

    NodeContainer nc_s2_r1;
    nc_s2_r1.Add(s2);
    nc_s2_r1.Add(r1);
    NetDeviceContainer dev_s2_r1 = p2p_s2Tor1.Install(nc_s2_r1);

    NodeContainer nc_r1_r3;
    nc_r1_r3.Add(r1);
    nc_r1_r3.Add(r3);
    NetDeviceContainer dev_r1_r3 = p2p_r1Tor3.Install(nc_r1_r3);

    NodeContainer nc_r2_r3;
    nc_r2_r3.Add(r2);
    nc_r2_r3.Add(r3);
    NetDeviceContainer dev_r2_r3 = p2p_r2Tor3.Install(nc_r2_r3);

    NodeContainer nc_r3_d;
    nc_r3_d.Add(r3);
    nc_r3_d.Add(d);
    NetDeviceContainer dev_r3_d = p2p_r3Tod.Install(nc_r3_d);

    Ptr<PointToPointNetDevice> router2HostDev = DynamicCast<PointToPointNetDevice> (dev_r3_d.Get (0));
    router2HostDev->SetAttribute ("DataRate", StringValue (dataRate));

    Ptr<Queue> dropTail = CreateObject<DropTailQueue> ();
    router2HostDev->SetQueue (dropTail);

    /* Install the IP stack. */
    InternetStackHelper internetStackH;
    internetStackH.Install(s);
    internetStackH.Install(s2);
    internetStackH.Install(r1);
    internetStackH.Install(r2);
    internetStackH.Install(r3);
    internetStackH.Install(d);

    /* IP assign. */
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_s_r1 = ipv4.Assign(dev_s_r1);

    ipv4.SetBase("10.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_s2_r1 = ipv4.Assign(dev_s2_r1);

    ipv4.SetBase("10.0.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_s_r2 = ipv4.Assign(dev_s_r2);
    ipv4.SetBase("10.0.2.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_r1_r3 = ipv4.Assign(dev_r1_r3);
    ipv4.SetBase("10.0.3.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_r2_r3 = ipv4.Assign(dev_r2_r3);
    ipv4.SetBase("10.0.4.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_r3_d = ipv4.Assign(dev_r3_d);

    /* Generate Route. */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    //

    nodeS = s.Get(0);
    Names::Add ("source", nodeS);
    nodeS2 = s2.Get(0);
    Names::Add ("source2", nodeS2);
    nodeD = d.Get(0);
    Names::Add ("destination", nodeD);
    routerR1 = r1.Get(0);
    Names::Add ("router1", routerR1);
    routerR2 = r2.Get(0);
    Names::Add ("router2", routerR2);
    routerR3 = r3.Get(0);
    Names::Add ("router3", routerR3);


    // Install Mobility Model
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector(5, 10, 0));
    positionAlloc->Add (Vector(5, 15, 0));
    positionAlloc->Add (Vector(10, 5, 0));
    positionAlloc->Add (Vector(10, 15, 0));
    positionAlloc->Add (Vector(15, 10, 0));
    positionAlloc->Add (Vector(20, 10, 0));

    MobilityHelper mobilityEnb;
    mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityEnb.SetPositionAllocator(positionAlloc);
    mobilityEnb.Install(nodeS);
    mobilityEnb.Install(nodeS2);
    mobilityEnb.Install(routerR1);
    mobilityEnb.Install(routerR2);
    mobilityEnb.Install(routerR3);
    mobilityEnb.Install(nodeD);

    uint32_t port = 5000;

    PrintNodeInfo(nodeS);
    PrintNodeInfo(nodeS2);
    PrintNodeInfo(nodeD);
    PrintNodeInfo(routerR3);

    MpTcpPacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodeD);
    sinkApps.Start(Seconds(startTime));
    sinkApps.Stop(Seconds(endTime));


    Ipv4Address dstaddr ("10.0.4.2");

    MpTcpBulkSendHelper sourceHelper("ns3::TcpSocketFactory", InetSocketAddress(dstaddr, port));
    sourceHelper.SetAttribute("MaxBytes", UintegerValue(4056000));
    ApplicationContainer sourceApp = sourceHelper.Install(nodeS);
    sourceApp.Start(Seconds(0));
    sourceApp.Stop(Seconds(endTime));

    MpTcpBulkSendHelper sourceHelper2("ns3::TcpSocketFactory", InetSocketAddress(dstaddr, port));
    sourceHelper2.SetAttribute("MaxBytes", UintegerValue(4056000));
    ApplicationContainer sourceApp2 = sourceHelper2.Install(nodeS2);
    sourceApp2.Start(Seconds(0));
    sourceApp2.Stop(Seconds(endTime));

    MpTcpBulkSendHelper sourceHelper3("ns3::TcpSocketFactory", InetSocketAddress(dstaddr, port));
    sourceHelper3.SetAttribute("MaxBytes", UintegerValue(4056000));
    ApplicationContainer sourceApp3 = sourceHelper3.Install(nodeS2);
    sourceApp3.Start(Seconds(0));
    sourceApp3.Stop(Seconds(endTime));

    MpTcpBulkSendHelper sourceHelper4("ns3::TcpSocketFactory", InetSocketAddress(dstaddr, port));
    sourceHelper4.SetAttribute("MaxBytes", UintegerValue(4056000));
    ApplicationContainer sourceApp4 = sourceHelper4.Install(nodeS2);
    sourceApp4.Start(Seconds(0));
    sourceApp4.Stop(Seconds(endTime));

    AnimationInterface anim ("sanidade.xml");
    anim.UpdateNodeColor(nodeS, 0, 0, 205);
    anim.UpdateNodeDescription(nodeS, "Source");
    anim.UpdateNodeColor(nodeD, 0, 100, 100);
    anim.UpdateNodeDescription(nodeD, "Destination");
    anim.UpdateNodeDescription(routerR1, "R1");
    anim.UpdateNodeDescription(routerR2, "R2");
    anim.UpdateNodeDescription(routerR3, "R3");

    //p2p_sTor1.EnablePcapAll ("p2p_0.pcap");
    //p2p_sTor2.EnablePcapAll ("p2p_1.pcap",true);

    AsciiTraceHelper h_ascii;


    /* Flow Monitor Configuration */
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    monitor->CheckForLostPackets();

    Gnuplot2dDataset dat1,dat2;
    Gnuplot2dDataset gd1,gd2;

    GnuplotHelper plotHelper;
    plotHelper.ConfigurePlot("plot-test",
                             "Title",
                             "xLegend",
                             "ylegend",
                             "png");

    plotHelper.PlotProbe("ns3::Ipv4PacketProbe",
                         "/NodeList/*/$ns3::Ipv4L3Protocol/Tx",
                         "OutputBytes-p1",
                         "Packet Byte Count",
                         GnuplotAggregator::KEY_BELOW);


    Ptr<MpTcpPacketSink> sink1 = DynamicCast<MpTcpPacketSink> (sinkApps.Get (0));
    GoodputMonitor(&flowmon, monitor,sink1,filename, delta);
    std::string everyDropTrFileName  = "sanidade-drops.dat";


    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->SerializeToXmlFile ("sanidade_flowmon.xml", true, true);



    Simulator::Destroy();
    NS_LOG_LOGIC("MpTcpNewReno:: simulation ended");
    return 0;
}
