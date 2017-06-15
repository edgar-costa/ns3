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


using namespace ns3;

std::string fileNameRoot = "ecmp_example";    // base name for trace files, etc
std::string outputNameRoot = "outputs/" + fileNameRoot;

NS_LOG_COMPONENT_DEFINE (fileNameRoot);

const char * file_name = g_log.Name();
std::string script_name = file_name;

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
  m_socket->Send (packet);
  //NS_LOG_UNCOND("Packet sent" << m_packetsSent);
  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
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
	NS_LOG_UNCOND (counter++ << " " << s << " at " << Simulator::Now ().GetSeconds ());
}

int
main (int argc, char *argv[])
{
  //Enable logging
	//LogComponentEnable("Ipv4GlobalRouting", LOG_DEBUG);
	//LogComponentEnable("Ipv4GlobalRouting", LOG_ERROR);
  LogComponentEnable("ecmp_example", LOG_ALL);


  //Command line arguments

  std::string ecmpMode = "ECMP_NONE";
  std::string protocol = "TCP";

  CommandLine cmd;
  cmd.AddValue ("EcmpMode", "EcmpMode: (0 none, 1 random, 2 flow, 3 Round_Robin)", ecmpMode);
  cmd.AddValue("Protocol", "Socket protocol used TCP or UDP. Default is "+protocol , protocol);
  cmd.Parse (argc, argv);

  //Here we should set how things are routed at the ipv4global routing module
  //TODO

  //Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue("True"));
  Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue(ecmpMode));
//  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

  //Create nodes
  NodeContainer nodes;
  nodes.Create (8);

  //Locate nodes for plotting
  for (uint32_t i = 0; i < nodes.GetN(); i++){
  	// Creates location object
    Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel>();
    // Aggregates position to node object
    nodes.Get(i)->AggregateObject(loc);

    if (i == 0){
    	loc->SetPosition(Vector(0,5,0));
    }
    else if (i == 1){
    	loc->SetPosition(Vector(2,5,0));
    }
    else if (i == 6) {
    	loc->SetPosition(Vector(8,5,0));
    }
    else if (i==7){
    	loc->SetPosition(Vector(10,5,0));
    }
    else{
    	loc->SetPosition(Vector(5,1+(2.66*(i-2)),0));
    }
  }

  //Define the csma channel

  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue ("10Mbps"));
  csma.SetChannelAttribute("Delay", StringValue ("2ms"));
  csma.SetChannelAttribute("FullDuplex", BooleanValue("True"));
  csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
  csma.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(100));

  //Define point to point
//  PointToPointHelper csma;
//
//
//  // create point-to-point link from A to R
//  csma.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("10Mbps")));
//  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(2)));

  //Install a csma device and channel in our hosts pairs

  NetDeviceContainer n0n1 = csma.Install (NodeContainer(nodes.Get(0),nodes.Get(1)));
  NetDeviceContainer n1n2 = csma.Install (NodeContainer(nodes.Get(1),nodes.Get(2)));
  NetDeviceContainer n1n3 = csma.Install (NodeContainer(nodes.Get(1),nodes.Get(3)));
  NetDeviceContainer n1n4 = csma.Install (NodeContainer(nodes.Get(1),nodes.Get(4)));
  NetDeviceContainer n1n5 = csma.Install (NodeContainer(nodes.Get(1),nodes.Get(5)));
  NetDeviceContainer n2n6 = csma.Install (NodeContainer(nodes.Get(2),nodes.Get(6)));
  NetDeviceContainer n3n6 = csma.Install (NodeContainer(nodes.Get(3),nodes.Get(6)));
  NetDeviceContainer n4n6 = csma.Install (NodeContainer(nodes.Get(4),nodes.Get(6)));
  NetDeviceContainer n5n6 = csma.Install (NodeContainer(nodes.Get(5),nodes.Get(6)));
  NetDeviceContainer n6n7 = csma.Install (NodeContainer(nodes.Get(6),nodes.Get(7)));

  NetDeviceContainer net_containers[] = {n0n1, n1n2, n1n3, n1n4, n1n5, n2n6, n3n6, n4n6, n5n6, n6n7};

  // Install Internet stack and assing ips
  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address("10.0.1.0", "255.255.255.0");
  for (uint32_t i=0; i < 10; i++){
  	address.Assign(net_containers[i]);
  	address.NewNetwork();
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("node1_tables", std::ios::out);
  Ipv4GlobalRoutingHelper::PrintRoutingTableAt(Seconds(1), nodes.Get(1), routingStream);

  NS_LOG_UNCOND(GetNodeIp(nodes.Get(6)));
  NS_LOG_UNCOND(GetNodeIp(nodes.Get(7)));

  //Prepare sink app
  uint16_t sinkPort = 8082;
  Ptr<Socket> ns3Socket;
  //had to put an initial value
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps;

  if (protocol == "TCP")
  {
    ns3Socket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (7));
  }
  else
	{
  	packetSinkHelper.SetAttribute("Protocol",StringValue("ns3::UdpSocketFactory"));
    ns3Socket = Socket::CreateSocket (nodes.Get (0), UdpSocketFactory::GetTypeId ());
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (7));
	}

  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (20.));

  //Prepare sender
  Ipv4Address addr = GetNodeIp(nodes.Get(7));

  Address sinkAddress (InetSocketAddress (addr, sinkPort));

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3Socket, sinkAddress, 1200, 550, DataRate ("10Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (20.));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (outputNameRoot+".cwnd");
  if (protocol == "TCP"){
  	ns3Socket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
  }

  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (outputNameRoot+".pcap", std::ios::out, PcapHelper::DLT_PPP);


  n0n1.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));


  n0n1.Get (0)->TraceConnectWithoutContext ("PhyTxDrop", MakeBoundCallback (&TxDrop, "PhyTxDrop"));
  n0n1.Get (0)->TraceConnectWithoutContext ("MacTxDrop", MakeBoundCallback (&TxDrop, "MacTxDrop" ));
  n0n1.Get (0)->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&TxDrop, "MacTx"));

  csma.EnablePcapAll (outputNameRoot);

  //Animation
  AnimationInterface anim("ecmp_test");
  anim.SetMaxPktsPerTraceFile(10000000);
  for (uint32_t i = 1; i < 7; i++)
        anim.UpdateNodeColor(nodes.Get(i), 0, 128, 0);
  anim.EnablePacketMetadata(true);


  Simulator::Stop (Seconds (20));
  Simulator::Run ();
  Simulator::Destroy ();


  return 0;
}

