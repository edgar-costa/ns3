/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "utils.h"

NS_LOG_COMPONENT_DEFINE ("Utils");

namespace ns3 {

/* ... */


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

//Returns node if added to the name system , 0 if it does not exist
Ptr<Node> GetNode(std::string name){
	return Names::Find<Node>(name);
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
  		NS_LOG_UNCOND("Pos: " << router_name.str() << " " << edge_pos);


//  		Allocate hosts
  		for (int host_i= 0; host_i < k/2 ; host_i++){
  			Ptr<ConstantPositionMobilityModel> loc1 = CreateObject<ConstantPositionMobilityModel>();

    		std::stringstream host_name;
    		host_name << "h_" << pod << "_" << (host_i + (k/2*pod_router));

    		double hosts_distance = 1.6;
    		host_pos.x = edge_pos.x -(hosts_distance/2) + (host_i * (hosts_distance/((k/2)-1)));

    		GetNode(host_name.str())->AggregateObject(loc1);
    		loc1->SetPosition(host_pos);
    		NS_LOG_UNCOND("Pos: " << host_name.str() << " " << host_pos);

  		}

			//Allocate aggregations
			Ptr<ConstantPositionMobilityModel> loc2 = CreateObject<ConstantPositionMobilityModel>();

  		router_name.str("");
  		router_name << "r_" << pod << "_a" << pod_router;

  		//Update edge pos
  		agg_pos.x = (agg_pos.x + 3);

  		GetNode(router_name.str())->AggregateObject(loc2);
  		loc2->SetPosition(agg_pos);
  		NS_LOG_UNCOND("Pos: " << router_name.str() << " " << agg_pos);

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
		NS_LOG_UNCOND("Pos: " << router_name.str() << " " << core_pos);


	}

}


}

