#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("FirstExample");

int main(int argc, char *argv[])
{
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create (50);

    // create link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

    // create internet stack
    InternetStackHelper internet;
    internet.Install (nodes);

    Ipv4AddressHelper ipv4;

//	Can be used for routing
//  Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",     BooleanValue(true)); // enable multi-path routing
//  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    for (uint32_t y = 0; y < 5; y++) {
        // install application for each server
        UdpEchoServerHelper echoServer(9999);
        ApplicationContainer serverApps = echoServer.Install (nodes.Get(y));
        serverApps.Start (Seconds (1.0));
        serverApps.Stop (Seconds (1000.0));

		for (uint32_t i = y * 5 + 5; i < y * 5 + 10; i++) {
			NetDeviceContainer devices = p2p.Install(nodes.Get(y), nodes.Get(i));
			std::string s = "10.";
			std::ostringstream s1;
			s1 << y;
			s.append(s1.str());
			s.append(".");
			std::ostringstream s2;
			s2 << i;
			s.append(s2.str());
			s.append(".0");
			ipv4.SetBase (ns3::Ipv4Address(s.c_str()), "255.255.255.0");
			Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

			UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9999);
			echoClient.SetAttribute ("MaxPackets", UintegerValue (200));
			echoClient.SetAttribute ("Interval", TimeValue (Seconds (1)));
			echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
			ApplicationContainer clientApps = echoClient.Install (nodes.Get(i));
			clientApps.Start (Seconds (2.0));
			clientApps.Stop (Seconds (10.0));
		}
    }

// 	  dump config
//    p2p.EnablePcapAll ("test");

    AnimationInterface anim("anim_first.xml");

    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
