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
#include <unordered_map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/traffic-generation-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/utils.h"


using namespace ns3;

std::string fileNameRoot = "fat-tree";    // base name for trace files, etc
std::string outputNameRoot = "outputs/" + fileNameRoot;

NS_LOG_COMPONENT_DEFINE (fileNameRoot);

const char * file_name = g_log.Name();
std::string script_name = file_name;

int counter = 0;





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


  //Command line arguments
  std::string ecmpMode = "ECMP_NONE";
  std::string protocol = "TCP";
  std::string experimentName = "default";
  uint16_t sinkPort = 8582;
  uint32_t num_packets = 100;
  uint16_t packet_size = 1400;
  uint16_t queue_size = 20;
  int64_t flowlet_gap = 50;

  bool animation = false;
  bool monitor = false;
  bool debug = false;

  //Fat tree parameters
  int k =  4;
  DataRate dataRate("10Mbps");

  CommandLine cmd;
  cmd.AddValue("Animation", "Enable animation module" , animation);
  cmd.AddValue("Monitor", "" , monitor);
  cmd.AddValue("Debug", "" , debug);



  cmd.AddValue("ExperimentName", "Name of the experiment: " , experimentName);


  cmd.AddValue("Protocol", "Protocol used by the traffic Default is: "+protocol , protocol);
  cmd.AddValue("SinkPort", "Sink port", sinkPort);
  cmd.AddValue("NumPackets", "Sink port", num_packets);
  cmd.AddValue("PacketSize", "Sink port", packet_size);

  cmd.AddValue ("EcmpMode", "EcmpMode: (0 none, 1 random, 2 flow, 3 Round_Robin)", ecmpMode);
	cmd.AddValue("dataRate", "Bandwidth of link, used in multiple experiments", dataRate);
  cmd.AddValue("QueueSize", "Sink port", queue_size);
  cmd.AddValue("FlowletGap", "Sink port", flowlet_gap);
  cmd.AddValue("K", "Fat tree size", k);

  cmd.Parse (argc, argv);

	if (debug){
		//LogComponentEnable("Ipv4GlobalRouting", LOG_DEBUG);
		//LogComponentEnable("Ipv4GlobalRouting", LOG_ERROR);
		LogComponentEnable("fat-tree", LOG_ERROR);
		LogComponentEnable("utils", LOG_ERROR);
		//LogComponentEnable("traffic-generation", LOG_DEBUG);
	}

  //Update root name
  outputNameRoot = outputNameRoot + "-" + experimentName;

  //General default configurations

  //Routing
  Config::SetDefault("ns3::Ipv4GlobalRouting::EcmpMode", StringValue(ecmpMode));
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));
  Config::SetDefault("ns3::Ipv4GlobalRouting::FlowletGap", IntegerValue(MilliSeconds(flowlet_gap).GetNanoSeconds()));

  //TCP
	uint32_t minRTO = 100;
	int delay = 10;
	int rtt = 12*delay;

  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1446));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1500000000));
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1500000000));

  //still have to understand this one by one
	Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue(MicroSeconds(rtt)));
	Config::SetDefault ("ns3::TcpSocketBase::MinRto",TimeValue(MilliSeconds (minRTO)));
	Config::SetDefault ("ns3::TcpSocketBase::MaxSegLifetime",DoubleValue(0));
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1446));
	Config::SetDefault ("ns3::TcpSocket::DataRetries", UintegerValue (5000));
	Config::SetDefault ("ns3::TcpSocket::ConnCount",UintegerValue(5000));

	Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(MicroSeconds(2*rtt)));
	Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(2));
  //Config::SetDefault("ns3::TcpSocketBase::ReTxThreshold", UintegerValue(3));

  //Define Interfaces

  //Define the csma channel

//  CsmaHelper csma;
//  csma.SetChannelAttribute("DataRate", DataRateValue (dataRate));
//  csma.SetChannelAttribute("Delay", StringValue ("0.5ms"));
//  csma.SetChannelAttribute("FullDuplex", BooleanValue("True"));
//  csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
//  csma.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queue_size));

  //Define point to point
  PointToPointHelper csma;

//   create point-to-point link from A to R
  csma.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(0.5)));
  csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
  csma.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queue_size));


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
  		NS_LOG_DEBUG(host_name.str());

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
  	  NS_LOG_DEBUG(router_name.str());

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
  	  NS_LOG_DEBUG(router_name.str());

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
  	  NS_LOG_DEBUG(router_name.str());

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
  //Uninstall FIFO queue //  //uninstall qdiscs
  TrafficControlHelper tch;

  Ipv4AddressHelper address("10.0.1.0", "255.255.255.0");
  for (auto it : links){
  	address.Assign(it.second);
  	address.NewNetwork();
  	tch.Uninstall(it.second);
  }

  //Create a ip to node mapping
  std::unordered_map<std::string, Ptr<Node>> ipToNode;

  for (uint32_t host_i = 0; host_i < hosts.GetN(); host_i++){
  	Ptr<Node> host = hosts.Get(host_i);
  	ipToNode[ipv4AddressToString(GetNodeIp(host))] = host;
  }




  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//
//  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("node1_tables", std::ios::out);
//  Ipv4GlobalRoutingHelper::PrintRoutingTableAt(Seconds(1), nodes.Get(1), routingStream);
//
//

//START TRAFFIC


//  //Prepare sink app
  std::unordered_map <std::string, std::vector<uint16_t>> hostToPort = installSinks(hosts, 5, 1000 , protocol);

  startStride(hosts, hostToPort, dataRate, 2, k);



  //////////////////
  //TRACES
  ///////////////////
//  Ptr<Socket> sock = installSimpleSend(GetNode("h_0_0"), GetNode("h_1_0"), randomFromVector(hostToPort["h_1_0"]), dataRate, num_packets,protocol);
//  Ptr<Socket> sock2 = installSimpleSend(GetNode("h_0_1"), GetNode("h_1_0"), randomFromVector(hostToPort["h_1_0"]), dataRate, num_packets,protocol);
//  installSimpleSend(GetNode("h_3_1"), GetNode("h_2_0"), randomFromVector(hostToPort["h_2_0"]), dataRate, num_packets,protocol);



//   Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (outputNameRoot+".cwnd");
//   Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (outputNameRoot+"2.cwnd");
//  if (protocol == "TCP"){
//  	sock->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
//  	sock2->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream2));
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
  //links["h_0_0->r_0_e0"].Get (0)->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&TxDrop, "MacTx h_0_0"));
  //links["h_0_1->r_0_e0"].Get (0)->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&TxDrop, "MacTx h_0_1"));

//
  csma.EnablePcap(outputNameRoot, links["h_3_0->r_3_e0"].Get(0), bool(1));
  //csma.EnablePcap(outputNameRoot, links["h_0_1->r_0_e0"].Get(0), bool(1));



  //Allocate nodes in a fat tree shape
  if (animation){
  allocateNodesFatTree(k);

  //Animation
  AnimationInterface anim(outputNameRoot+".anim");
  anim.SetMaxPktsPerTraceFile(10000000);

//  setting colors
  for (uint32_t i = 0; i < hosts.GetN(); i++){
  			Ptr<Node> host = hosts.Get(i);
        anim.UpdateNodeColor(host, 0, 0, 255);
  			anim.UpdateNodeSize(host->GetId(), 0.5, 0.5);

  }
  for (uint32_t i = 0; i < edgeRouters.GetN(); i++)
  {
        anim.UpdateNodeColor(edgeRouters.Get(i), 0, 200, 0);
  			anim.UpdateNodeColor(aggRouters.Get(i), 0, 200, 0);
  }

  for (uint32_t i = 0; i < coreRouters.GetN(); i++)
        anim.UpdateNodeColor(coreRouters.Get(i), 255, 0, 0);

  anim.EnablePacketMetadata(true);
  }

  ////////////////////
  //Flow Monitor
  ////////////////////
  AsciiTraceHelper asciiTraceHelper;

  FlowMonitorHelper flowHelper;
  Ptr<FlowMonitor>  flowMonitor;
  Ptr<Ipv4FlowClassifier> classifier;
  std::map<FlowId, FlowMonitor::FlowStats> stats;

  if (monitor)
  {
  	flowMonitor = flowHelper.InstallAll ();
  }

  Simulator::Stop (Seconds (6000));
  Simulator::Run ();

  if (monitor){
		classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
		stats = flowMonitor->GetFlowStats ();

		//File where we write FCT
		Ptr<OutputStreamWrapper> fct = asciiTraceHelper.CreateFileStream (outputNameRoot+".fct");

		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i){

			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			double last_t = i->second.timeLastRxPacket.GetSeconds();
			double first_t = i->second.timeFirstTxPacket.GetSeconds();
			double duration = (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds());

			*fct->GetStream() << duration << " " << first_t << " " << last_t <<  "\n";

			std::cout << "Flow " << i->first << " (" << (GetNodeName(ipToNode[ipv4AddressToString(t.sourceAddress)])) << "-> " << GetNodeName(ipToNode[ipv4AddressToString(t.destinationAddress)]) << ")\n";

//			std::cout << "Tx Bytes:   " << i->second.txBytes << "\n";
//			std::cout << "Rx Bytes:   " << i->second.rxBytes << "\n";
			std::cout << "Flow duration:   " << duration << " " << first_t << " " << last_t <<  "\n";

			std::cout << "Throughput: " << i->second.rxBytes * 8.0 / duration /1024/1024  << " Mbps\n";}


//  flowHelper.SerializeToXmlFile (std::string("outputs/") + "flowmonitor", true, true);

  }


  Simulator::Destroy ();


  return 0;
}

