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
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/utils.h"


using namespace ns3;

std::string fileNameRoot = "fat-tree";    // base name for trace files, etc
std::string outputNameRoot = "outputs/" + fileNameRoot;

NS_LOG_COMPONENT_DEFINE (fileNameRoot);

const char * file_name = g_log.Name();
std::string script_name = file_name;

int counter = 0;



///// My appp

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
  		if (counter < 20000){
  			tNext  = Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()));
  			counter++;
  		}
  		else{
  			//NS_LOG_UNCOND("hola");
  			tNext =(Seconds(0.0001));
  			counter = 0;
    }
  	m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

//TRACE SINKS

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

	//INITIALIZATION

	//Set simulator's time resolution (click)
	Time::SetResolution (Time::NS);

	//Change that if i want to get different random values each run otherwise i will always get the same.
	RngSeedManager::SetRun (1);   // Changes run number from default of 1 to 7

  //Enable logging
	//LogComponentEnable("Ipv4GlobalRouting", LOG_DEBUG);
	//LogComponentEnable("Ipv4GlobalRouting", LOG_ERROR);
  LogComponentEnable("fat-tree", LOG_ALL);


  //Command line arguments
  std::string ecmpMode = "ECMP_NONE";
  std::string protocol = "TCP";
  uint16_t sinkPort = 8582;
  uint32_t num_packets = 100;
  uint16_t packet_size = 1400;
  uint16_t queue_size = 20;
  int64_t flowlet_gap = 50;


  //Fat tree parameters
  int k =  4;
  DataRate dataRate("10Mbps");

  CommandLine cmd;
  cmd.AddValue("Protocol", "Protocol used by the traffic Default is: "+protocol , protocol);
  cmd.AddValue("SinkPort", "Sink port", sinkPort);
  cmd.AddValue("NumPackets", "Sink port", num_packets);
  cmd.AddValue("PacketSize", "Sink port", packet_size);


	cmd.AddValue("dataRate", "Bandwidth of link, used in multiple experiments", dataRate);
  cmd.AddValue("QueueSize", "Sink port", queue_size);
  cmd.AddValue("FlowletGap", "Sink port", flowlet_gap);


  cmd.AddValue("K", "Fat tree size", k);


  cmd.Parse (argc, argv);

  //General default configurations

  //Routing
  Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue(ecmpMode));
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));
  Config::SetDefault("ns3::Ipv4GlobalRouting::FlowletGap", IntegerValue(MilliSeconds(flowlet_gap).GetNanoSeconds()));

  //TCP
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1446));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1500000000));
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1500000000));
  Config::SetDefault("ns3::TcpSocketBase::ReTxThreshold", UintegerValue(3));

  //Define Interfaces

  //Define the csma channel

  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", DataRateValue (dataRate));
  csma.SetChannelAttribute("Delay", StringValue ("0.5ms"));
  csma.SetChannelAttribute("FullDuplex", BooleanValue("True"));
  csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
  csma.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(100));

  //Define point to point
//  PointToPointHelper csma;

  // create point-to-point link from A to R
//  csma.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
//  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(0.5)));
//  csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
//  csma.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(1));


  //Compute Fat Tree Devices

  int num_pods = k;
  int num_hosts = k*k*k/4;

  int num_edges = k*k/2;
  int num_agg = k*k/2;
  int num_cores = k*k/4;

  int hosts_per_edge = k/2;
  int hosts_per_pod = k*k/4;
  int routers_per_layer = k/2;

  NS_LOG_DEBUG("Num_pods: " << k << " Num Hosts: " << num_hosts << " Num Edges: "
  		<< num_edges << " Num agg: " << num_agg << " Num Cores: " << num_cores
			<< " Hosts per pod: " << hosts_per_pod << "  Routers per layer: " << routers_per_layer);


  //Hosts
  NodeContainer hosts;
  hosts.Create(num_hosts);

  //Give names to hosts using names class
  int pod = 0; int inpod_num = 0; int host_count = 0; int router_count;

  for (NodeContainer::Iterator host = hosts.Begin(); host != hosts.End(); host++ ){

  		inpod_num =  host_count % hosts_per_pod;
  		pod = host_count / hosts_per_pod;

  		std::stringstream host_name;
  		host_name << "h_" << pod << "_" << inpod_num;
  	  NS_LOG_UNCOND(host_name.str());

  		Names::Add(host_name.str(), (*host));

  		host_count++;
  }


  //Edge routers
  NodeContainer edgeRouters;
	edgeRouters.Create (num_edges);

  //Give names to hosts using names class
  pod = 0; inpod_num = 0; router_count = 0;
  for (NodeContainer::Iterator router = edgeRouters.Begin(); router != edgeRouters.End(); router++ ){

  		inpod_num =  router_count % routers_per_layer;
  		pod = router_count / routers_per_layer;

  		std::stringstream router_name;
  		router_name << "r_" << pod << "_e" << inpod_num;
  	  NS_LOG_UNCOND(router_name.str());

  		Names::Add(router_name.str(), (*router));

  		router_count++;
  }

  //Agg routers
  NodeContainer aggRouters;
	aggRouters.Create (num_agg);

  //Give names to hosts using names class
  pod = 0; inpod_num = 0; router_count = 0;
  for (NodeContainer::Iterator router = aggRouters.Begin(); router != aggRouters.End(); router++ ){

  		inpod_num =  router_count % routers_per_layer;
  		pod = router_count / routers_per_layer;

  		std::stringstream router_name;
  		router_name << "r_" << pod << "_a" << inpod_num;
  	  NS_LOG_UNCOND(router_name.str());

  		Names::Add(router_name.str(), (*router));

  		router_count++;
  }


  //Core routers
  NodeContainer coreRouters;
	coreRouters.Create (num_cores);

  //Give names to hosts using names class
  router_count = 0;
  for (NodeContainer::Iterator router = coreRouters.Begin(); router != coreRouters.End(); router++ ){

  		std::stringstream router_name;
  		router_name << "r_c"  <<router_count;
  	  NS_LOG_UNCOND(router_name.str());

  		Names::Add(router_name.str(), (*router));

  		router_count++;
  }


  //Install net devices between nodes (so add links) would be good to save them somewhere I could maybe use the namesystem or map
  //Install internet stack to nodes, very easy

  std::unordered_map<std::string, NetDeviceContainer> links;


  //Add links between fat tree nodes

  //Create PODs

  for (int pod_i=0; pod_i < num_pods; pod_i ++)
  {

  	//Connect edges with hosts
  	for (int edge_i=0; edge_i < routers_per_layer; edge_i++)
  	{
  		std::stringstream edge_name;
  		edge_name << "r_" << pod_i << "_e" << edge_i;

  		for (int host_i=0; host_i < hosts_per_edge; host_i++){

    		std::stringstream host_name;
    		host_name << "h_" << pod_i << "_" << (host_i + (hosts_per_edge*edge_i));

    		NS_LOG_DEBUG("Adding link between " << host_name.str() << " and " << edge_name.str());
  		  links[host_name.str()+"->"+edge_name.str()] = csma.Install (NodeContainer(GetNode(host_name.str()),GetNode(edge_name.str())));
  		}

  		//connect edge with all the agg

  		for (int agg_i = 0; agg_i < routers_per_layer; agg_i++){
    		std::stringstream agg_name;
    		agg_name << "r_" << pod_i << "_a" << agg_i;

    		NS_LOG_DEBUG("Adding link between " << edge_name.str() << " and " << agg_name.str());
  		  links[edge_name.str()+"->"+agg_name.str()] = csma.Install (NodeContainer(GetNode(edge_name.str()),GetNode(agg_name.str())));

  		}

  	}

  	//Connect agg with core layer

		for (int agg_i = 0; agg_i < routers_per_layer; agg_i++){
  		std::stringstream agg_name;
  		agg_name << "r_" << pod_i << "_a" << agg_i;

  		//Connect to every k/2 cores
  		for (int core_i=(agg_i* (k/2)); core_i < ((agg_i+1)*(k/2)) ; core_i++){
    		std::stringstream core_name;
    		core_name << "r_c" << core_i;

    		NS_LOG_DEBUG("Adding link between " << agg_name.str() << " and " << core_name.str());
  		  links[agg_name.str()+"->"+core_name.str()] = csma.Install (NodeContainer(GetNode(agg_name.str()),GetNode(core_name.str())));
  		}

  	}
  }

  // Install Internet stack and assing ips
  InternetStackHelper stack;
  stack.Install (hosts);
  stack.Install (edgeRouters);
  stack.Install (aggRouters);
  stack.Install (coreRouters);


  //Assign IPS

  Ipv4AddressHelper address("10.0.1.0", "255.255.255.0");
  for (auto it : links){
  	address.Assign(it.second);
  	address.NewNetwork();
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();




//
//  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("node1_tables", std::ios::out);
//  Ipv4GlobalRoutingHelper::PrintRoutingTableAt(Seconds(1), nodes.Get(1), routingStream);
//
//
//  //Prepare sink app
  Ptr<Socket> ns3Socket;
  //had to put an initial value
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps;

  if (protocol == "TCP")
  {
    ns3Socket = Socket::CreateSocket (GetNode("h_0_0"), TcpSocketFactory::GetTypeId ());
    ApplicationContainer sinkApps = packetSinkHelper.Install (GetNode("h_1_0"));
  }
  else
	{
  	packetSinkHelper.SetAttribute("Protocol",StringValue("ns3::UdpSocketFactory"));
    ns3Socket = Socket::CreateSocket (GetNode("h_0_0"), UdpSocketFactory::GetTypeId ());
    ApplicationContainer sinkApps = packetSinkHelper.Install (GetNode("h_1_0"));
	}

  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (1000.));


  Ipv4Address addr = GetNodeIp("h_1_0");

  Address sinkAddress (InetSocketAddress (addr, sinkPort));

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3Socket, sinkAddress, 1440, 100, DataRate ("10Mbps"));
  GetNode("h_0_0")->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (1000.));


//
//  AsciiTraceHelper asciiTraceHelper;
//  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (outputNameRoot+".cwnd");
//  if (protocol == "TCP"){
//  	ns3Socket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
//  }
//
//  PcapHelper pcapHelper;
//  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (outputNameRoot+".pcap", std::ios::out, PcapHelper::DLT_PPP);
//
//
//  n0n1.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));
//
//
//  n0n1.Get (0)->TraceConnectWithoutContext ("PhyTxDrop", MakeBoundCallback (&TxDrop, "PhyTxDrop"));
//  n0n1.Get (0)->TraceConnectWithoutContext ("MacTxDrop", MakeBoundCallback (&TxDrop, "MacTxDrop" ));
//  n0n1.Get (0)->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&TxDrop, "MacTx"));
//
//  csma.EnablePcapAll (outputNameRoot);

//  //Animation
//  AnimationInterface anim("ecmp_test");
//  anim.SetMaxPktsPerTraceFile(10000000);
//  for (uint32_t i = 1; i < 12; i++)
//        anim.UpdateNodeColor(nodes.Get(i), 0, 128, 0);
//  anim.EnablePacketMetadata(true);


  Simulator::Stop (Seconds (1000));
  Simulator::Run ();

  NS_LOG_UNCOND("counter: " << counter);


  Simulator::Destroy ();


  return 0;
}

