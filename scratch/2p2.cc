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

//       10.0.0.0
//       ----------
//    n0            n1
//       ----------
//       10.0.1.0

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("MpTcpNewReno");

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


Ptr<Node> client;
Ptr<Node> server;
Ptr<Node> router0;
Ptr<Node> router1;





int
main(int argc, char *argv[])
{
    /* Uncoupled_TCPs, Linked_Increases, RTT_Compensator, Fully_Coupled */
    Config::SetDefault("ns3::MpTcpSocketBase::CongestionControl", StringValue("RTT_Compensator"));
    Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(636));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(MpTcpSocketBase::GetTypeId()));
    Config::SetDefault("ns3::MpTcpSocketBase::MaxSubflows", UintegerValue(8)); // Sink
    
    //LogComponentEnable("MpTcpNewReno", LOG_LEVEL_ALL);
    //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
    //LogComponentEnable("MpTcpSocketBase", LOG_INFO);
    
    /* Build nodes. */
    NodeContainer term_0;
    term_0.Create(1);
    NodeContainer term_1;
    term_1.Create(1);
    
    NodeContainer router_0;
    router_0.Create(1);
    NodeContainer router_1;
    router_1.Create(1);
    
    /* Build link. */
    PointToPointHelper p2p0;
    p2p0.SetDeviceAttribute("DataRate", DataRateValue(100000));
    p2p0.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));
    
    PointToPointHelper p2p1;
    p2p1.SetDeviceAttribute("DataRate", DataRateValue(100000));
    p2p1.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));
    
    PointToPointHelper p2p2;
    p2p2.SetDeviceAttribute("DataRate", DataRateValue(100000));
    p2p2.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));
    
    PointToPointHelper p2p3;
    p2p3.SetDeviceAttribute("DataRate", DataRateValue(100000));
    p2p3.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));
    
    /* Build link net device container. */
    NodeContainer t0_r0;
    t0_r0.Add(term_0);
    t0_r0.Add(router_0);
    NetDeviceContainer dT0_r0 = p2p0.Install(t0_r0);
   
    NodeContainer t0_r1;
    t0_r1.Add(term_0);
    t0_r1.Add(router_1);
    NetDeviceContainer dT0_r1 = p2p1.Install(t0_r1);
    
    NodeContainer r0_t1;
    r0_t1.Add(router_0);
    r0_t1.Add(term_1);
    NetDeviceContainer dR0_t1 = p2p2.Install(r0_t1);
    
    NodeContainer r1_t1;
    r1_t1.Add(router_1);
    r1_t1.Add(term_1);
    NetDeviceContainer dR1_t1 = p2p3.Install(r1_t1);
    
    
    /* Install the IP stack. */
    InternetStackHelper internetStackH;
    internetStackH.Install(term_0);
    internetStackH.Install(term_1);
    internetStackH.Install(router_0);
    internetStackH.Install(router_1);
    
    /* IP assign. */
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_ndc_p2p_0 = ipv4.Assign(dT0_r0);
    ipv4.SetBase("10.0.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_ndc_p2p_1 = ipv4.Assign(dT0_r1);
    ipv4.SetBase("10.0.2.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_ndc_p2p_2 = ipv4.Assign(dR0_t1);
    ipv4.SetBase("10.0.3.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_ndc_p2p_3 = ipv4.Assign(dR1_t1);
    
    /* Generate Route. */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    //
    
    client = term_0.Get(0);
    Names::Add ("client", client);
    server = term_1.Get(0);
    Names::Add ("server", server);
    router0 = router_0.Get(0);
    Names::Add ("router0", router0);
    router1 = router_1.Get(0);
    Names::Add ("router1", router1);
   
    // Install Mobility Model
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector(5, 0, 0));
    positionAlloc->Add (Vector(0, 5, 0));
    positionAlloc->Add (Vector(10, 5, 0));
    positionAlloc->Add (Vector(5, 10, 0));
    
    MobilityHelper mobilityEnb;
    mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityEnb.SetPositionAllocator(positionAlloc);
    mobilityEnb.Install(client);
    mobilityEnb.Install(router0);
    mobilityEnb.Install(router1);
    mobilityEnb.Install(server);

    uint32_t port = 5000;
    
    PrintNodeInfo(client);
    PrintNodeInfo(server);
    
    MpTcpPacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(server);
    sinkApps.Start(Seconds(0));
    sinkApps.Stop(Seconds(3));
    
    Ipv4Address dstaddr ("10.0.2.2");
    
    MpTcpBulkSendHelper sourceHelper("ns3::TcpSocketFactory", InetSocketAddress(dstaddr, port));
    sourceHelper.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApp = sourceHelper.Install(client);
    sourceApp.Start(Seconds(0));
    sourceApp.Stop(Seconds(3));

    AnimationInterface anim ("p2p.xml");
    
    p2p0.EnablePcapAll ("p2p_0.pcap");
    p2p1.EnablePcapAll ("p2p_1.pcap",true);
    
    Simulator::Stop(Seconds(4.0));
    Simulator::Run();
    
    Simulator::Destroy();
    NS_LOG_LOGIC("MpTcpNewReno:: simulation ended");
    return 0;
}