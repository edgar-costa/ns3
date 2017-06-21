/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

namespace ns3 {

/* ... */

Ipv4Address GetNodeIp(Ptr<Node> node);
std::string ipToString(uint8_t first,uint8_t second, uint8_t third, uint8_t fourth);
Ptr<Node> GetNode(std::string name);

}

#endif /* UTILS_H */

