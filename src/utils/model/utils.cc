/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "utils.h"

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
allocateNodesFatTree(int k, NodeContainer hosts, NodeContainer edgeRouters, NodeContainer aggRouters, NodeContainer coreRouters){


//  Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel>();
//  GetNode(node_name)->AggregateObject(loc);
//	loc->SetPosition(Vector(2,5,0));

	//Locate edge and agg
	Vector edge_pos(3,4,0);
	Vector agg_pos (3,6.5,0);
	Vector host_pos(0,0,0);

	for (int pod= 0; pod < k ; pod++){

		for (int pod_router = 0; pod_router < k/2; pod_router++){

			//Allocate edges
			Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel>();

  		std::stringstream router_name;
  		router_name << "r_" << pod << "_e" << pod_router;

  		//Update edge pos
  		edge_pos.x = (edge_pos.x + (5*pod) + (2.5*pod_router));

  		GetNode(router_name.str())->AggregateObject(loc);
  		loc->SetPosition(edge_pos);

  		//Allocate hosts
  		for (int host_i; host_i < k/2 ; host_i++){
  			Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel>();

    		std::stringstream host_name;
    		router_name << "h_" << pod << "_" << (host_i + (k/2*pod_router));

    		host_pos.x = edge_pos.x;
    		host_pos.y = edge_pos.y - 2.5;

  		}


			//Allocate aggregations
			Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel>();

  		std::stringstream router_name;
  		router_name << "r_" << pod << "_a" << pod_router;

  		//Update edge pos
  		agg_pos.x = (agg_pos.x + (5*pod) + (2.5*pod_router));

  		GetNode(router_name.str())->AggregateObject(loc);
  		loc->SetPosition(edge_pos);

		}

	}


}


}

