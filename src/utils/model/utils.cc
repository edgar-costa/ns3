/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "utils.h"

NS_LOG_COMPONENT_DEFINE ("utils");



namespace ns3 {

/* ... */

Ptr<UniformRandomVariable> random_variable = CreateObject<UniformRandomVariable> ();


Ipv4Address
GetNodeIp(std::string node_name)
{

	Ptr<Node> node = GetNode(node_name);

  Ptr<Ipv4> ip = node->GetObject<Ipv4> ();

  ObjectVectorValue interfaces;
  NS_ASSERT(ip !=0);
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

//* Returns the amount of bytes needed to send at dataRate during time seconds.
uint64_t BytesFromRate(DataRate dataRate, double time){

		double bytes = ((double)dataRate.GetBitRate()/8) * time;
		NS_LOG_DEBUG("Bytes to send: " << bytes);
		return bytes;
}

uint64_t hash_string(std::string message){
  Hasher hasher;
  hasher.clear();
  return hasher.GetHash64(message);

}

std::vector< std::pair<double,uint64_t>> GetDistribution(std::string distributionFile) {

  std::vector< std::pair<double,uint64_t>> cumulativeDistribution;
  std::ifstream infile(distributionFile);

  NS_ASSERT_MSG(infile, "Please provide a valid file for reading the flow size distribution!");
  double cumulativeProbability;
  int size;
  while (infile >> size >> cumulativeProbability)
  {
    cumulativeDistribution.push_back(std::make_pair(cumulativeProbability, size));
  }

//  for(uint32_t i = 0; i < cumulativeDistribution.size(); i++)
//  {
//    NS_LOG_FUNCTION(cumulativeDistribution[i].first << " " << cumulativeDistribution[i].second);
//  }

  return cumulativeDistribution;
}

//*
//Get size from cumulative traffic distribution
//

uint64_t GetFlowSizeFromDistribution(std::vector< std::pair<double,uint64_t>> distribution, double uniformSample){

  NS_ASSERT_MSG(uniformSample <= 1.0, "Provided sampled number is bigger than 1.0!");

  uint64_t size =  1500; //at least 1 packet

  uint64_t previous_size = 0;
  double previous_prob = 0.0;

  for (uint32_t i=0; i < distribution.size(); i++){
  	if (uniformSample <= distribution[i].first){

  		//compute the proportional size depending on the position
  		if (i > 0){
  			previous_size = distribution[i-1].second;
  			previous_prob = distribution[i-1].first;
  		}
  		double relative_distance = (uniformSample - previous_prob)/(distribution[i].first - previous_prob);
  		NS_LOG_UNCOND(relative_distance << " " << uniformSample << " " << previous_prob << " " << distribution[i].first << " " << previous_size);
  		size = previous_size + (relative_distance * (distribution[i].second - previous_size));

  		break;
  	}
  }
  return size;
}

//Assume a fat tree and the following naming h_x_y
std::pair<uint16_t, uint16_t> GetHostPositionPair(std::string name){

	std::stringstream text(name);
	std::string segment;
	std::pair<uint16_t, uint16_t> result;

	std::getline(text, segment, '_');
	std::getline(text, segment, '_');
	result.first = (uint16_t)std::stoi(segment);
	std::getline(text, segment, '_');
	result.second = (uint16_t)std::stoi(segment);

	return result;
}


Ipv4Address
GetNodeIp(Ptr<Node> node)
{

  Ptr<Ipv4> ip = node->GetObject<Ipv4> ();

  ObjectVectorValue interfaces;
  NS_ASSERT(ip !=0);
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

// Function to create address string from numbers
//
std::string ipToString(uint8_t first,uint8_t second, uint8_t third, uint8_t fourth)
{
	std::string address = std::to_string(first) + "." + std::to_string(second) + "." + std::to_string(third) + "." + std::to_string(fourth);

	return address;
}

 std::string ipv4AddressToString(Ipv4Address address){

	 std::stringstream ip;
	 ip << address;
	 return ip.str();

//  	return ipToString((address.m_address & 0xff000000) >> 24, (address.m_address & 0x00ff0000) >> 16, (address.m_address & 0x0000ff00) >> 8, address.m_address & 0x000000ff);
 }

//Returns node if added to the name system , 0 if it does not exist
Ptr<Node> GetNode(std::string name){
	return Names::Find<Node>(name);
}

std::string GetNodeName(Ptr<Node> node){
	return Names::FindName(node);
}

void
allocateNodesFatTree(int k){


//  Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel>();
//  GetNode(node_name)->AggregateObject(loc);
//	loc->SetPosition(Vector(2,5,0));

	//Locate edge and agg
	Vector host_pos(0,30,0);
	Vector edge_pos(0,25,0);
	Vector agg_pos (0,19.5,0);

	for (int pod= 0; pod < k ; pod++){

		for (int pod_router = 0; pod_router < k/2; pod_router++){

			//Allocate edges
			Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel>();

  		std::stringstream router_name;
  		router_name << "r_" << pod << "_e" << pod_router;

  		//Update edge pos
  		edge_pos.x = (edge_pos.x + 3);

  		GetNode(router_name.str())->AggregateObject(loc);
  		loc->SetPosition(edge_pos);
  		NS_LOG_DEBUG("Pos: " << router_name.str() << " " << edge_pos);


//  		Allocate hosts
  		for (int host_i= 0; host_i < k/2 ; host_i++){
  			Ptr<ConstantPositionMobilityModel> loc1 = CreateObject<ConstantPositionMobilityModel>();

    		std::stringstream host_name;
    		host_name << "h_" << pod << "_" << (host_i + (k/2*pod_router));

    		double hosts_distance = 1.6;
    		host_pos.x = edge_pos.x -(hosts_distance/2) + (host_i * (hosts_distance/((k/2)-1)));

    		GetNode(host_name.str())->AggregateObject(loc1);
    		loc1->SetPosition(host_pos);
    		NS_LOG_DEBUG("Pos: " << host_name.str() << " " << host_pos);

  		}

			//Allocate aggregations
			Ptr<ConstantPositionMobilityModel> loc2 = CreateObject<ConstantPositionMobilityModel>();

  		router_name.str("");
  		router_name << "r_" << pod << "_a" << pod_router;

  		//Update edge pos
  		agg_pos.x = (agg_pos.x + 3);

  		GetNode(router_name.str())->AggregateObject(loc2);
  		loc2->SetPosition(agg_pos);
  		NS_LOG_DEBUG("Pos: " << router_name.str() << " " << agg_pos);

		}
		edge_pos.x = edge_pos.x + 3;
		agg_pos.x = agg_pos.x + 3;

	}

	//Allocate Core routers
	int num_cores = (k/2) * (k/2);
	int offset = 6;
	double distance = (edge_pos.x -offset*2);
	double step = distance/(num_cores-1);
	Vector core_pos_i(offset,10,0);
	Vector core_pos(offset,10,0);


	for (int router_i = 0; router_i < num_cores; router_i++){

		Ptr<ConstantPositionMobilityModel> loc3 = CreateObject<ConstantPositionMobilityModel>();

		std::stringstream router_name;
		router_name << "r_c" << router_i;

		//Update edge pos
		core_pos.x =core_pos_i.x + (router_i * step);

		GetNode(router_name.str())->AggregateObject(loc3);
		loc3->SetPosition(core_pos);
		NS_LOG_DEBUG("Pos: " << router_name.str() << " " << core_pos);


	}

}

void printNow(double delay){
	NS_LOG_UNCOND("\nSimulation Time: " << Simulator::Now().GetSeconds() << "\n");
	Simulator::Schedule (Seconds(delay), &printNow, delay);
}

}
