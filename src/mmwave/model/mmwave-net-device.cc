/*
 * mmwave-net-device.cc
 *
 *  Created on: Nov 5, 2014
 *      Author: sourjya
 */

#include "ns3/simulator.h"
#include "ns3/callback.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/ipv4-l3-protocol.h"
#include "mmwave-net-device.h"


namespace ns3{

NS_LOG_COMPONENT_DEFINE ("MmWaveNetDevice");

NS_OBJECT_ENSURE_REGISTERED (MmWaveNetDevice);

TypeId MmWaveNetDevice::GetTypeId ()
{
	static TypeId
	    tid =
	    TypeId ("ns3::MmWaveNetDevice")
	    .SetParent<NetDevice> ()
		.AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
					   UintegerValue (30000),
					   MakeUintegerAccessor (&MmWaveNetDevice::SetMtu,
											 &MmWaveNetDevice::GetMtu),
					   MakeUintegerChecker<uint16_t> ())
;

	return tid;
}

MmWaveNetDevice::MmWaveNetDevice (void)
{
  NS_LOG_FUNCTION (this);
}


MmWaveNetDevice::~MmWaveNetDevice (void)
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveNetDevice::DoDispose (void)
{
	m_node = 0;
	NetDevice::DoDispose ();
}

void
MmWaveNetDevice::SetIfIndex (const uint32_t index)
{
	m_ifIndex = index;
}
uint32_t
MmWaveNetDevice::GetIfIndex (void) const
{
	return m_ifIndex;
}
Ptr<Channel>
MmWaveNetDevice::GetChannel (void) const
{
	return 0;
}
void
MmWaveNetDevice::SetAddress (Address address)
{
	NS_LOG_FUNCTION (this << address);
	m_macaddress = Mac48Address::ConvertFrom (address);
}
Address
MmWaveNetDevice::GetAddress (void) const
{
	NS_LOG_FUNCTION (this);
	return m_macaddress;
}
bool
MmWaveNetDevice::SetMtu (const uint16_t mtu)
{
	m_mtu = mtu;
	return true;
}
uint16_t
MmWaveNetDevice::GetMtu (void) const
{
	return m_mtu;
}
bool
MmWaveNetDevice::IsLinkUp (void) const
{
	return m_linkUp;
}
void
MmWaveNetDevice::AddLinkChangeCallback (Callback<void> callback)
{

}
bool
MmWaveNetDevice::IsBroadcast (void) const
{
	return false;
}
Address
MmWaveNetDevice::GetBroadcast (void) const
{
	return Mac48Address::GetBroadcast ();
}
bool
MmWaveNetDevice::IsMulticast (void) const
{
	return false;
}
Address
MmWaveNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
	return Mac48Address ("01:00:5e:00:00:00");
}
bool
MmWaveNetDevice::IsBridge (void) const
{
	return false;
}
bool
MmWaveNetDevice::IsPointToPoint (void) const
{
	return false;
}

bool
MmWaveNetDevice::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
	NS_FATAL_ERROR ("Send from not supported");
	return false;
}

Ptr<Node>
MmWaveNetDevice::GetNode (void) const
{
	return m_node;
}

void
MmWaveNetDevice::SetNode (Ptr<Node> node)
{
	m_node = node;
}

bool
MmWaveNetDevice::NeedsArp (void) const
{
	return false;
}

Address
MmWaveNetDevice::GetMulticast (Ipv6Address addr) const
{
	Address dummy;
	return dummy;
}

void
MmWaveNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
	NS_LOG_FUNCTION (this);
	m_rxCallback = cb;
}

void
MmWaveNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{

}

bool
MmWaveNetDevice::SupportsSendFrom (void) const
{
	return false;
}

void
MmWaveNetDevice::Receive (Ptr<Packet> p)
{
	NS_LOG_FUNCTION (this << p);
	m_rxCallback (this, p, Ipv4L3Protocol::PROT_NUMBER, Address ());
}

bool
MmWaveNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
	bool ret = DoSend ( packet, dest, protocolNumber);
	return ret;
}

Ipv4Address
MmWaveNetDevice::GetPacketDestination (Ptr<Packet> packet)
{
	Ipv4Address dest_ip;
	Ptr<Packet> q = packet->Copy();

	Ipv4Header ipHeader;
	q->PeekHeader (ipHeader);
	dest_ip = ipHeader.GetDestination();
	return dest_ip;
}


}


