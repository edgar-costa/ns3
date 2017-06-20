/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/traffic-control-module.h"


using namespace ns3;

std::string fileNameRoot = "channel_test";    // base name for trace files, etc
std::string outputNameRoot = "outputs/" + fileNameRoot;

NS_LOG_COMPONENT_DEFINE (fileNameRoot);

const char * file_name = g_log.Name();
std::string script_name = file_name;

int totalSent = 0;



Ipv4Address
GetNodeIp(Ptr<Node> node)
{
  Ptr<Ipv4> ip = node->GetObject<Ipv4> ();

  NS_ASSERT(ip !=0);
  ObjectVectorValue interfaces;
  ip->GetAttribute("InterfaceList", interfaces);
  for(ObjectVectorValue::Iterator j = interfaces.Begin(); j != interfaces.End (); j ++)
  {
    Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
    NS_ASSERT(ipIface != 0);
    Ptr<NetDevice> device = ipIface->GetDevice();
    NS_ASSERT(device != 0);
    Ipv4Address ipAddr = ipIface->GetAddress (0).GetLocal();

    // ignore localhost interface...
    if (ipAddr == Ipv4Address("127.0.0.1")) {
      continue;
    }
    else {
      return ipAddr;
    }
  }

  return Ipv4Address("127.0.0.1");
}


class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  Socket::SocketType            m_socket_type;

};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0),
		m_socket_type (Socket::NS3_SOCK_STREAM)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
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
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);

  int packets_sent = m_socket->Send (packet);

  if (packets_sent == -1){
  	NS_LOG_UNCOND("Failed sending Socket at " << Simulator::Now().GetSeconds());
  }
  else{
  	totalSent = totalSent + packets_sent;
  }

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
	Time tNext;
  if (m_running)
    {
  			tNext  = Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()));

  	m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
//  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newCwnd << std::endl;
}

static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  //NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
}

static void
TxDrop (std::string s, Ptr<const Packet> p){
	static int counter = 0;
	NS_LOG_UNCOND (counter++ << " " << s << " at " << Simulator::Now ().GetSeconds ()) ;
}

int
main (int argc, char *argv[])
{

	Time::SetResolution (Time::NS);

	//Change that if i want to get different random values each run otherwise i will always get the same.
	RngSeedManager::SetRun (1);   // Changes run number from default of 1 to 7

  //Enable logging
	//LogComponentEnable("Ipv4GlobalRouting", LOG_DEBUG);
	//LogComponentEnable("Ipv4GlobalRouting", LOG_ERROR);
  LogComponentEnable("channel_test", LOG_ALL);


  //Command line arguments
  std::string protocol = "TCP";
  uint16_t sinkPort = 8582;

  uint32_t num_packets = 100;
  uint16_t packet_size = 1400;
  uint16_t queue_size = 20;

  CommandLine cmd;
  cmd.AddValue("Protocol", "Socket protocol used TCP or UDP. Default is "+protocol , protocol);
  cmd.AddValue("SinkPort", "Sink port", sinkPort);
  cmd.AddValue("NumPackets", "Sink port", num_packets);
  cmd.AddValue("PacketSize", "Sink port", packet_size);
  cmd.AddValue("QueueSize", "Sink port", queue_size);


  cmd.Parse (argc, argv);

  //Here we should set how things are routed at the ipv4global routing module
  //TODO

  //Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue("True"));
//  Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue(ecmpMode));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1430));
//  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1500000000));
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1500000000));
  Config::SetDefault("ns3::TcpSocketBase::ReTxThreshold", UintegerValue(3));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));



  //Create nodes
  NodeContainer nodes;
  nodes.Create (3);

  //Define the csma channel
//
//  CsmaHelper csma;
//  csma.SetChannelAttribute("DataRate", StringValue ("100Mbps"));
//  csma.SetChannelAttribute("Delay", StringValue ("0.5ms"));
//  csma.SetChannelAttribute("FullDuplex", BooleanValue("True"));
//  csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
//  csma.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queue_size));

  //Define point to point
  PointToPointHelper csma;

//create point-to-point link from A to R
  csma.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(0.5)));
  csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
  csma.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queue_size));

  //Install a csma device and channel in our hosts pairs

  NetDeviceContainer n0n1 = csma.Install (NodeContainer(nodes.Get(0),nodes.Get(1)));
  NetDeviceContainer n1n2 = csma.Install (NodeContainer(nodes.Get(1),nodes.Get(2)));


  NetDeviceContainer net_containers[] = {n0n1, n1n2};


  // Install Internet stack and assing ips
  InternetStackHelper stack;
  stack.Install (nodes);

  Ptr<Node> n1 = nodes.Get(0);
  Ptr<NetDevice> p2p = n1->GetDevice(0);


  PointerValue p2p_queue;
  p2p->GetAttribute("TxQueue", p2p_queue);
  Ptr<Queue> txQueue = p2p_queue.Get<Queue>();

  UintegerValue limit;
  txQueue->GetAttribute("MaxPackets", limit);
  NS_LOG_UNCOND("queue " << limit.Get());

  int array_length = sizeof(net_containers)/sizeof(net_containers[0]);
  NS_LOG_UNCOND(array_length);

  Ipv4AddressHelper address("10.0.1.0", "255.255.255.0");
  for (int i=0; i < array_length; i++){
  	address.Assign(net_containers[i]);
  	address.NewNetwork();
  }
//
//  //uninstall qdiscs
  TrafficControlHelper tch;
  tch.Uninstall(n0n1);
  tch.Uninstall(n1n2);


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("node1_tables", std::ios::out);
//  Ipv4GlobalRoutingHelper::PrintRoutingTableAt(Seconds(1), nodes.Get(1), routingStream);


  //Prepare sink app
  Ptr<Socket> ns3Socket;
  //had to put an initial value
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps;

  if (protocol == "TCP")
  {
    ns3Socket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (2));
  }
  else
	{
  	packetSinkHelper.SetAttribute("Protocol",StringValue("ns3::UdpSocketFactory"));
    ns3Socket = Socket::CreateSocket (nodes.Get (0), UdpSocketFactory::GetTypeId ());
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (3));
	}

  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (1000.));

  //Prepare sender
  Ipv4Address addr = GetNodeIp(nodes.Get(2));

  Address sinkAddress (InetSocketAddress (addr, sinkPort));

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3Socket, sinkAddress, packet_size, num_packets, DataRate ("100Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (1000.));


  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (outputNameRoot+".cwnd");

  if (protocol == "TCP"){
  	ns3Socket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
  }

//  PcapHelper pcapHelper;
//  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (outputNameRoot+".pcap", std::ios::out, PcapHelper::DLT_PPP);
//
//
//  n0n1.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));


  n0n1.Get (0)->TraceConnectWithoutContext ("PhyTxDrop", MakeBoundCallback (&TxDrop, "PhyTxDrop"));
  n0n1.Get (0)->TraceConnectWithoutContext ("MacTxDrop", MakeBoundCallback (&TxDrop, "MacTxDrop" ));
  n0n1.Get (0)->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&TxDrop, "MacTx"));

  csma.EnablePcapAll (outputNameRoot);



  Simulator::Stop (Seconds (1000));
  Simulator::Run ();

  NS_LOG_UNCOND("Total bytes Sent: " << totalSent);

  Simulator::Destroy ();


  return 0;
}

