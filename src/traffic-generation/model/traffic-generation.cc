/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "traffic-generation.h"
#include "ns3/applications-module.h"
#include "ns3/utils.h"

namespace ns3 {

/* ... */
///// My appp


SimpleSend::SimpleSend ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0),
		m_socket_type (Socket::NS3_SOCK_STREAM),
	  m_counter(0)
{
}

SimpleSend::~SimpleSend ()
{
  m_socket = 0;
}

/* static */
TypeId SimpleSend::GetTypeId (void)
{
  static TypeId tid = TypeId ("SimpleSend")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<SimpleSend> ()
    ;
  return tid;
}

void
SimpleSend::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
SimpleSend::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
SimpleSend::StopApplication (void)
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
SimpleSend::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);

  int packets_sent = m_socket->Send (packet);

  if (packets_sent == -1){
  	NS_LOG_UNCOND("Failed sending Socket at " << Simulator::Now().GetSeconds());
  }

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
  else{
  	m_socket->Close();
  }

}

void
SimpleSend::ScheduleTx (void)
{
	Time tNext;
  if (m_running)
    {
  		if (m_counter < 5000){
  			tNext  = Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()));
  			m_counter++;
  		}
  		else{
  			//NS_LOG_UNCOND("Delayed");
  			//tNext = Seconds(0.02);
  			tNext = MilliSeconds(11);
  			m_counter = 0;
    }
  	m_sendEvent = Simulator::Schedule (tNext, &SimpleSend::SendPacket, this);
    }
}

//NEW TRAFFIC GENERATION TOOLS

void installSink(Ptr<Node> node, uint16_t sinkPort, uint32_t duration, std::string protocol){

  //create sink helper
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));

  if (protocol == "UDP"){
  	packetSinkHelper.SetAttribute("Protocol",StringValue("ns3::UdpSocketFactory"));
	}

  ApplicationContainer sinkApps = packetSinkHelper.Install(node);

  sinkApps.Start (Seconds (0));
  sinkApps.Stop (Seconds (duration));
}



//
//std::unordered_map <std::string, std::vector<uint16_t>> installSinks(NodeContainer hosts, uint16_t sinksPerHost, uint32_t duration, std::string protocol){
//
//	std::unordered_map <std::string, std::vector<uint16_t>> hostsToPorts;
//  Ptr<UniformRandomVariable> random_generator = CreateObject<UniformRandomVariable> ();
//  uint16_t starting_port;
//
//  for(ObjectVectorValue::Iterator host = hosts.Begin(); host != hosts.End (); host ++){
//
//  	starting_port = random_generator->GetInteger(0, (uint16_t)-1 - sinksPerHost);
//  	std::string host_name = GetNodeName((*host).second);
//
//  	hostsToPorts[host_name] = std::vector();
//
//  	for (int i = 0; i < sinksPerHost ; i++){
//  		installSink((*host).second, starting_port+i, duration, protocol);
//  		//Add port into the vector
//    	hostsToPorts[host_name].push_back(starting_port+i);
//  	}
//
//  }
//
//  return hostsToPorts;
//}
//
//
//
//void startStride(NodeContainer hosts, std::unordered_map <std::string, std::vector<uint16_t>> hostsToPorts, DataRate sendingRate){
//
//	uint16_t vector_size = hostsToPorts.begin()->second.size();
//
//
//	for (auto host = hosts.Begin(); host != hosts.End(); host++){
//		return;
//	}

//}



}

