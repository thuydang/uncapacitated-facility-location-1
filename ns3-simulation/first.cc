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
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <map>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("FirstExample");
const int NUM_OF_SERVERS = 10;
const int NUM_OF_NODES = 50;

vector<string> allCaches;
vector<string> combination;
vector<vector<string>> allCombinations;

class Client {
public:
	string nodeKey;
	string cacheKey;

	Client(string nodeKey) {
		this->nodeKey = nodeKey;
		cacheKey = "";
	}
};

class Cache {
public:
	string nodeKey;
	int initialCost;
	int totalOffer;
	bool isOpen;

	Cache(string nodeKey) {
		this->nodeKey = nodeKey;
		initialCost = 5;
		totalOffer = 0;
		isOpen = false;
	}
};

int getClosestServerIdx(int nodeIdx, int NUM_OF_SERVERS, int pos[][2]) {
	double minDistance = std::numeric_limits<double>::max();
	int closestServerIdx = 0;

	for (uint8_t i = 0; i < NUM_OF_SERVERS; i++) {
		double curDistance = sqrt(pow(pos[nodeIdx][0] - pos[i][0], 2) + pow(pos[nodeIdx][1] - pos[i][1], 2));
		if (curDistance < minDistance) {
			minDistance = curDistance;
			closestServerIdx = i;
		}
	}
	return closestServerIdx;
}

vector<string> simulate(vector<Client> clients, vector<Cache> caches, map<string, map<string, int>> numberOfHops) {
	uint32_t lenClients = clients.size();
	uint32_t lenCaches = caches.size();
	int v[lenClients];
	std::fill(v, v + lenClients, 0);

	int w[lenCaches][lenClients][2];
	for (uint32_t i = 0; i < lenCaches; i++) {
		for (uint32_t k = 0; k < lenClients; k++) {
			w[i][k][0] = 0;
			w[i][k][1] = 0;
		}
	}

	bool hasUnconnectedClient = false;
	for (uint32_t i = 0; i < lenClients; i++) {
		if (clients[i].cacheKey == "") {
			hasUnconnectedClient = true;
			break;
		}
	}

	int time = 0;
	while (hasUnconnectedClient) {
		time++;
		for (uint32_t j = 0; j < lenClients; j++) {
			if (clients[j].cacheKey == "") {
				v[j]++;
			}
			for (uint32_t i = 0; i < lenCaches; i++) {
				ostringstream i2Str, j2Str;
				i2Str << caches[i].nodeKey;
				j2Str << clients[j].nodeKey;
				if (!caches[i].isOpen && clients[j].cacheKey == "" && numberOfHops[j2Str.str()][i2Str.str()] == v[j]) {
					w[i][j][1] = 1;
				}
				if (clients[j].cacheKey == "" && w[i][j][1] == 1) {
					w[i][j][0]++;
					caches[i].totalOffer++;
				}
				if (!caches[i].isOpen && caches[i].initialCost == caches[i].totalOffer) {
					caches[i].isOpen = true;
					for (uint32_t j2 = 0; j2 < lenClients; j2++) {
						if (w[i][j2][0] > 0) {
							clients[j2].cacheKey = caches[i].nodeKey;
							for (uint32_t i2 = 0; i2 < lenCaches; i2++) {
								ostringstream i22Str, j22Str;
								i22Str << caches[i2].nodeKey;
								j22Str << clients[j2].nodeKey;
								int temp = w[i2][j2][0];
								int diff = numberOfHops[j22Str.str()][i2Str.str()] - numberOfHops[j22Str.str()][i22Str.str()];
								w[i2][j2][0] = max(0, diff);
								caches[i2].totalOffer += w[i2][j2][0] - temp;
							}
						}
					}
				}
				if (caches[i].isOpen && clients[j].cacheKey == "" && numberOfHops[j2Str.str()][i2Str.str()] == v[j]) {
					clients[j].cacheKey = caches[i].nodeKey;
					for (uint32_t i2 = 0; i2 < lenCaches; i2++) {
						ostringstream i22Str;
						i22Str << caches[i2].nodeKey;
						int temp = w[i2][j][0];
						int diff = numberOfHops[j2Str.str()][i2Str.str()] - numberOfHops[j2Str.str()][i22Str.str()];
						w[i2][j][0] = max(0, diff);
						caches[i2].totalOffer += w[i2][j][0] - temp;
					}
				}
			}

		}

		hasUnconnectedClient = false;
		for (uint32_t i = 0; i < lenClients; i++) {
			if (clients[i].cacheKey == "") {
				hasUnconnectedClient = true;
				break;
			}
		}
		printf("Time: %d\n", time);
	}

	vector<string> client2Cache;
	for (uint32_t i = 0; i < lenClients; i++) {
		client2Cache.push_back(clients[i].cacheKey);
	}
	return client2Cache;
}

vector<string> findShortestPath(std::map<string, vector<string>> graphDict, string start, string end, vector<string> path) {
	path.push_back(start);
	if (start == end) {
		return path;
	}
	if (graphDict.find(start) == graphDict.end()) {
		return vector<string>();
	}
	vector<string> shortest;
	for (uint8_t i = 0; i < graphDict[start].size(); i++) {
		string node = graphDict[start][i];
		if (std::find(path.begin(), path.end(), node) == path.end()) {
			vector<string> newPath = findShortestPath(graphDict, node, end, path);
			if (!newPath.empty() && (shortest.empty() || newPath.size() < shortest.size())) {
				shortest = newPath;
			}
		}
	}
	return shortest;
}

void getCombinations(int offset, int k) {
	if (k == 0) {
		allCombinations.push_back(combination);
		return;
	}
	for (uint32_t i = offset; i <= allCaches.size() - k; i++) {
		combination.push_back(allCaches[i]);
		getCombinations(i+1, k-1);
		combination.pop_back();
	}
}

vector<int> getAllCosts(vector<Client> clients, vector<Cache> caches, map<string, map<string, int>> c) {
	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		getCombinations(0, i+1);
	}
	vector<int> allCosts;
	for (uint32_t i = 0; i < allCombinations.size(); i++) {
		vector<string> D2 (clients.size());
		fill(D2.begin(), D2.end(), "");
		for (uint32_t j = 0; j < D2.size(); j++) {
			for (uint32_t k = 0; k < caches.size(); k++) {
				for (uint32_t l = 0; l < allCombinations[i].size(); l++) {
					if (caches[k].nodeKey == allCombinations[i][l]) {
						if (D2[j] == "" || c[caches[k].nodeKey][clients[j].nodeKey] < c[D2[j]][clients[j].nodeKey])
							D2[j] = caches[k].nodeKey;
					}
				}
			}
		}

		int cost = 0;
		for (uint32_t j = 0; j < allCombinations[i].size(); j++) {
			cost += caches[j].initialCost;
		}
		for (uint32_t j = 0; j < D2.size(); j++) {
			if (D2[j] != "") {
				cost += c[D2[j]][clients[j].nodeKey];
			}
		}
		if (cost > 0)
			allCosts.push_back(cost);
	}
	return allCosts;
}

int getTotalCost(vector<Cache> caches, map<string, map<string, int>> c, vector<string> client2Cache) {
	int cost = 0;
	for (uint32_t i = 0; i < caches.size(); i++) {
		cost += caches[i].initialCost;
	}
	for (uint32_t i = 0; i < client2Cache.size(); i++) {
		if (client2Cache[i] != "") {
			ostringstream i2Str;
			i2Str << i;
			cost += c[i2Str.str()][client2Cache[i]];
		}
	}
	return cost;
}

int main(int argc, char *argv[])
{
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(NUM_OF_NODES);
    // create link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

    // create internet stack
    InternetStackHelper internet;
    internet.Install (nodes);

    Ipv4AddressHelper ipv4;
	Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    vector<Cache> caches;
    vector<Client> clients;

    int adjMatrix[NUM_OF_SERVERS][NUM_OF_SERVERS];
    for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
        for (uint32_t j = 0; j < NUM_OF_SERVERS; j++) {
        	adjMatrix[i][j] = 0;
        }
    }

	srand(1);
	bool again = true;
	vector<int> randCandidates;
	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		randCandidates.push_back(i);
	}

	uint32_t ipCounter3 = 0, ipCounter2 = 0, ipCounter1 = 0, ipCounter0 = 0;
	while (again) {
		for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
			uint32_t randIdx = rand() % randCandidates.size();
			uint32_t randInt = randCandidates[randIdx];
			if (i != randInt && adjMatrix[i][randInt] == 0) {
				ostringstream i2Str, randIntToStr;
				i2Str << i;
				randIntToStr << randInt;
				NetDeviceContainer devices = p2p.Install(nodes.Get(i), nodes.Get(randInt));

				if (ipCounter1 < 255)
					ipCounter1++;
				else if (ipCounter2 < 255)
					ipCounter2++;
				else
					ipCounter3++;
				ostringstream ipStream;
				ipStream << ipCounter3;
				ipStream << ".";
				ipStream << ipCounter2;
				ipStream << ".";
				ipStream << ipCounter1;
				ipStream << ".";
				ipStream << ipCounter0;

				ipv4.SetBase (ns3::Ipv4Address(ipStream.str().c_str()), "255.255.255.0");
				Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
				Ipv4GlobalRoutingHelper::RecomputeRoutingTables();

				adjMatrix[i][randInt] = 1;
				adjMatrix[randInt][i] = 1;
				for (uint32_t k = 0; k < NUM_OF_SERVERS; k++) {
					if (k != randInt && adjMatrix[i][k] != 0 && adjMatrix[k][randInt] != 1) {
						adjMatrix[k][randInt] = 2;
						adjMatrix[randInt][k] = 2;

					}
				}
			}
		}

		again = false;
		for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
			bool hasPath = true;
			for (uint32_t k = 0; k < NUM_OF_SERVERS; k++) {
				if (i != k && adjMatrix[i][k] == 0) {
					hasPath = false;
					again = true;
				}
			}
			if (hasPath) {
				std::vector<int>::iterator position = std::find(randCandidates.begin(), randCandidates.end(), i);
				if (position != randCandidates.end())
					randCandidates.erase(position);
			}
		}
	}

	int positions[NUM_OF_NODES][2];
	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		uint32_t rand_x = rand() % 50 + 25;
		uint32_t rand_y = rand() % 50 + 25;
		positions[i][0] = rand_x;
		positions[i][1] = rand_y;

		ostringstream i2Str;
		i2Str << i;
		caches.push_back(Cache(i2Str.str()));

		UdpEchoServerHelper echoServer(9999);
		ApplicationContainer serverApps = echoServer.Install (nodes.Get(i));
		serverApps.Start (Seconds (1.0));
		serverApps.Stop (Seconds (1000.0));
	}

	for (uint32_t i = NUM_OF_SERVERS; i < NUM_OF_NODES; i++) {
		uint32_t rand_x = rand() % 100;
		uint32_t rand_y = rand() % 100;
		positions[i][0] = rand_x;
		positions[i][1] = rand_y;
	}

	map<string, vector<string>> graphDict;
	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		vector<string> adjVector;
		ostringstream i2Str;
		i2Str << i;
		for (uint32_t j = 0; j < NUM_OF_SERVERS; j++) {
			if (adjMatrix[i][j] == 1) {
				ostringstream j2Str;
				j2Str << j;
				adjVector.push_back(j2Str.str());
			}
		}
		graphDict[i2Str.str()] = adjVector;
	}

	map<string, map<string, int>> numberOfHops;
	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		ostringstream i2Str;
		i2Str << i;
		for (uint32_t j = 0; j < NUM_OF_SERVERS; j++) {
			ostringstream j2Str;
			j2Str << j;
			numberOfHops[i2Str.str()][j2Str.str()] = findShortestPath(graphDict, i2Str.str(), j2Str.str(), vector<string>()).size();
		}
	}

	for (uint32_t i = NUM_OF_SERVERS; i < NUM_OF_NODES; i++) {
		int closestServerIdx = getClosestServerIdx(i, NUM_OF_SERVERS, positions);

		ostringstream server2Str;
		server2Str << closestServerIdx;
		clients.push_back(Client(server2Str.str()));
	}

	vector<string> client2Cache = simulate(clients, caches, numberOfHops);

	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		ostringstream i2Str;
		i2Str << i;
		allCaches.push_back(i2Str.str());
	}

	// Evaluate all costs and compare it with the best cost found by the algorithm
	/*
	vector<int> allCosts = getAllCosts(clients, caches, numberOfHops);
	sort(allCosts.begin(), allCosts.end());
	int totalCost = getTotalCost(caches, numberOfHops, client2Cache);
	cout << totalCost;
	*/

	for (uint32_t i = NUM_OF_SERVERS; i < NUM_OF_NODES; i++) {
		istringstream nodeKeyStr(clients[i-NUM_OF_SERVERS].nodeKey);
		int nodeKey;
		nodeKeyStr >> nodeKey;
		NetDeviceContainer devices = p2p.Install(nodes.Get(i), nodes.Get(nodeKey));

		if (ipCounter1 < 255)
			ipCounter1++;
		else if (ipCounter2 < 255)
			ipCounter2++;
		else
			ipCounter3++;
		ostringstream ipStream;
		ipStream << ipCounter3;
		ipStream << ".";
		ipStream << ipCounter2;
		ipStream << ".";
		ipStream << ipCounter1;
		ipStream << ".";
		ipStream << ipCounter0;

		ipv4.SetBase (ns3::Ipv4Address(ipStream.str().c_str()), "255.255.255.0");
		Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

		if (client2Cache[i-NUM_OF_SERVERS] != nodeKeyStr.str()) {
			vector<string> path = findShortestPath(graphDict, nodeKeyStr.str(), client2Cache[i-NUM_OF_SERVERS], vector<string>());
			// TODO use keys instead of indexes
			istringstream cacheKeyStr(client2Cache[i-NUM_OF_SERVERS]);
			int cacheKey;
			cacheKeyStr >> cacheKey;
			NetDeviceContainer devices2 = p2p.Install(nodes.Get(nodeKey), nodes.Get(cacheKey));
			ostringstream s3, s4;

			if (ipCounter1 < 255)
				ipCounter1++;
			else if (ipCounter2 < 255)
				ipCounter2++;
			else
				ipCounter3++;
			ostringstream ipStream;
			ipStream << ipCounter3;
			ipStream << ".";
			ipStream << ipCounter2;
			ipStream << ".";
			ipStream << ipCounter1;
			ipStream << ".";
			ipStream << ipCounter0;

			ipv4.SetBase (ns3::Ipv4Address(ipStream.str().c_str()), "255.255.255.0");
			Ipv4InterfaceContainer interfaces2 = ipv4.Assign (devices2);
			Ipv4GlobalRoutingHelper::RecomputeRoutingTables();

			UdpEchoClientHelper echoClient(interfaces2.GetAddress(1), 9999);
			echoClient.SetAttribute ("MaxPackets", UintegerValue(200));
			echoClient.SetAttribute ("Interval", TimeValue(Seconds (1)));
			echoClient.SetAttribute ("PacketSize", UintegerValue(1024));
			ApplicationContainer clientApps = echoClient.Install(nodes.Get(i));
			clientApps.Start(Seconds(2.0));
			clientApps.Stop(Seconds(10.0));
		}
		else {
			Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
			UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9999);
			echoClient.SetAttribute ("MaxPackets", UintegerValue(200));
			echoClient.SetAttribute ("Interval", TimeValue(Seconds (1)));
			echoClient.SetAttribute ("PacketSize", UintegerValue(1024));
			ApplicationContainer clientApps = echoClient.Install(nodes.Get(i));
			clientApps.Start(Seconds(2.0));
			clientApps.Stop(Seconds(10.0));
		}
	}

// 	  dump config
//    p2p.EnablePcapAll ("test");

    AnimationInterface anim("anim_first.xml");
	for (uint8_t i = 0; i < NUM_OF_SERVERS; i++) {
		anim.SetConstantPosition(nodes.Get(i), positions[i][0], positions[i][1]);
		anim.UpdateNodeColor(i, 255, 0, 0);
	}
	for (uint8_t i = NUM_OF_SERVERS; i < NUM_OF_NODES; i++) {
		anim.SetConstantPosition(nodes.Get(i), positions[i][0], positions[i][1]);
		anim.UpdateNodeColor(i, 255, 255, 255);

		istringstream cacheKeyStr(client2Cache[i - NUM_OF_SERVERS]);
		int cacheKey;
		cacheKeyStr >> cacheKey;
		anim.UpdateNodeColor(cacheKey, 0, 255, 0);
	}

    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
