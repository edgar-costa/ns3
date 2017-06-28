/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef TRAFFIC_GENERATION_H
#define TRAFFIC_GENERATION_H


#include <string.h>
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include <unordered_map>
#include <vector>

namespace ns3 {

class SimpleSend : public Application
{
public:
  SimpleSend ();
  virtual ~SimpleSend ();

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
  uint64_t        m_counter;
};

void installSink(Ptr<Node> node, uint16_t sinkPort, uint32_t duration, std::string protocol);
std::unordered_map <std::string, std::vector<uint16_t>> installSinks(NodeContainer hosts, uint16_t sinksPerHost, uint32_t duration, std::string protocol);
void startStride(NodeContainer hosts, std::unordered_map <std::string, std::vector<uint16_t>> hostsToPorts, DataRate sendingRate);

}

#endif /* TRAFFIC_GENERATION_H */

