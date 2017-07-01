/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "traffic-generation.h"
#include "simple-send.h"
#include "ns3/applications-module.h"
#include "ns3/utils.h"

NS_LOG_COMPONENT_DEFINE ("traffic-generation");


namespace ns3 {

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



Ptr<Socket> installSimpleSend(Ptr<Node> srcHost, Ptr<Node> dstHost, uint16_t sinkPort, DataRate dataRate, uint32_t numPackets, std::string protocol){

  Ptr<Socket> ns3Socket;
  uint32_t num_packets = numPackets;

  //had to put an initial value
  if (protocol == "TCP")
  {
    ns3Socket = Socket::CreateSocket (srcHost, TcpSocketFactory::GetTypeId ());
  }
  else
	{
    ns3Socket = Socket::CreateSocket (srcHost, UdpSocketFactory::GetTypeId ());
	}


  Ipv4Address addr = GetNodeIp(dstHost);

  Address sinkAddress (InetSocketAddress (addr, sinkPort));

  Ptr<SimpleSend> app = CreateObject<SimpleSend> ();
  app->Setup (ns3Socket, sinkAddress, 1440, num_packets, dataRate);

  srcHost->AddApplication (app);

  app->SetStartTime (Seconds (1.));
  //Stop not needed since the simulator stops when there are no more packets to send.
  //app->SetStopTime (Seconds (1000.));
  return ns3Socket;
}

//DO THE SAME WITH THE BULK APP, WHICH IS PROBABLY WHAT WE WANT TO HAVE.
void installBulkSend(){

}



std::unordered_map <std::string, std::vector<uint16_t>> installSinks(NodeContainer hosts, uint16_t sinksPerHost, uint32_t duration, std::string protocol){

	std::unordered_map <std::string, std::vector<uint16_t>> hostsToPorts;
  Ptr<UniformRandomVariable> random_generator = CreateObject<UniformRandomVariable> ();
  uint16_t starting_port;

  for(NodeContainer::Iterator host = hosts.Begin(); host != hosts.End (); host ++){

  	starting_port = random_generator->GetInteger(0, (uint16_t)-1 - sinksPerHost);
  	std::string host_name = GetNodeName((*host));

  	hostsToPorts[host_name] = std::vector<uint16_t>();

  	for (int i = 0; i < sinksPerHost ; i++){
  		installSink((*host), starting_port+i, duration, protocol);
  		//Add port into the vector
  		NS_LOG_DEBUG("Install Sink: " << host_name << " port:" << starting_port+i);
    	hostsToPorts[host_name].push_back(starting_port+i);
  	}

  }

  return hostsToPorts;
}


void startStride(NodeContainer hosts, std::unordered_map <std::string, std::vector<uint16_t>> hostsToPorts, DataRate sendingRate, uint16_t nFlows, uint16_t offset){

//	uint16_t vector_size = hostsToPorts.begin()->second.size();
	uint16_t numHosts =  hosts.GetN();

	uint16_t index = 0;
	for (NodeContainer::Iterator host = hosts.Begin(); host != hosts.End(); host++){

		//Get receiver
		Ptr<Node> dst = hosts.Get((index + offset) % numHosts);
		std::vector<uint16_t> availablePorts = hostsToPorts[GetNodeName(dst)];

		for (int flow_i =0; flow_i < nFlows; flow_i++){

			//get random available port
			uint16_t dport = randomFromVector<uint16_t>(availablePorts);

			//create sender
			NS_LOG_DEBUG("Start Sender: src:" << GetNodeName(*host) << " dst:" <<  GetNodeName(dst) << " dport:" << dport);
			installSimpleSend((*host), dst,	dport, sendingRate, 100, "TCP");
		}
		index++;
	}

}



}

