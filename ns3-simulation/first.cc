#include "ns3/core-module.h"
#include "ns3/network-module.h"
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
#include <numeric>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("FirstExample");
const int NUM_OF_SERVERS = 10;
const int NUM_OF_NODES = 100;
const int NUM_OF_REQUESTS_PER_CLIENT = 2;

vector<int> combination;
vector<vector<int>> allCombinations;
AnimationInterface* anim = 0;
uint32_t resourceOnlineCloudId;
uint32_t resourceOfflineCloudId;
uint32_t resourceClientId;

class Request {
public:
	int from, to;
	int cacheKey;

	Request() {
		this->from = -1;
		this->to = -1;
		cacheKey = -1;
	}

	Request(int from, int to) {
		this->from = from;
		this->to = to;
		cacheKey = -1;
	}
};

class Cache {
public:
	int nodeKey;
	int initialCost;
	int totalOffer;
	bool isOpen;

	Cache(int nodeKey) {
		this->nodeKey = nodeKey;
		initialCost = 5;
		totalOffer = 0;
		isOpen = false;
	}
};
vector<Cache> caches;

//int getClosestServerIdx(int nodeIdx, int NUM_OF_SERVERS, int pos[][2]) {
//	double minDistance = std::numeric_limits<double>::max();
//	int closestServerIdx = 0;
//
//	for (uint8_t i = 0; i < NUM_OF_SERVERS; i++) {
//		double curDistance = sqrt(pow(pos[nodeIdx][0] - pos[i][0], 2) + pow(pos[nodeIdx][1] - pos[i][1], 2));
//		if (curDistance < minDistance) {
//			minDistance = curDistance;
//			closestServerIdx = i;
//		}
//	}
//	return closestServerIdx;
//}

vector<Request> simulate(vector<Request> requests, vector<vector<int>> numberOfHops) {
	uint32_t lenRequests = requests.size();
	uint32_t lenCaches = caches.size();
	int v[lenRequests];
	std::fill(v, v + lenRequests, 0);

	int w[lenCaches][lenRequests][2];
	for (uint32_t i = 0; i < lenCaches; i++) {
		for (uint32_t k = 0; k < lenRequests; k++) {
			w[i][k][0] = 0;
			w[i][k][1] = 0;
		}
	}

	bool hasUnconnectedRequest = false;
	for (uint32_t i = 0; i < lenRequests; i++) {
		if (requests[i].cacheKey == -1) {
			hasUnconnectedRequest = true;
			break;
		}
	}

	int time = 0;
	while (hasUnconnectedRequest) {
		time++;
		for (uint32_t j = 0; j < lenRequests; j++) {
			if (requests[j].cacheKey == -1) {
				v[j]++;
			}
			for (uint32_t i = 0; i < lenCaches; i++) {
				if (!caches[i].isOpen && requests[j].cacheKey == -1 && numberOfHops[j][caches[i].nodeKey] == v[j]) {
					w[i][j][1] = 1;
				}
				if (requests[j].cacheKey == -1 && w[i][j][1] == 1) {
					w[i][j][0]++;
					caches[i].totalOffer++;
				}
				if (!caches[i].isOpen && caches[i].initialCost == caches[i].totalOffer) {
					caches[i].isOpen = true;
					for (uint32_t j2 = 0; j2 < lenRequests; j2++) {
						if (w[i][j2][0] > 0) {
							requests[j2].cacheKey = caches[i].nodeKey;
							for (uint32_t i2 = 0; i2 < lenCaches; i2++) {
								int temp = w[i2][j2][0];
								int diff = numberOfHops[j2][caches[i].nodeKey] - numberOfHops[j2][caches[i2].nodeKey];
								w[i2][j2][0] = max(0, diff);
								caches[i2].totalOffer += w[i2][j2][0] - temp;
							}
						}
					}
				}
				if (caches[i].isOpen && requests[j].cacheKey == -1 && numberOfHops[j][caches[i].nodeKey] == v[j]) {
					requests[j].cacheKey = caches[i].nodeKey;
					for (uint32_t i2 = 0; i2 < lenCaches; i2++) {
						int temp = w[i2][j][0];
						int diff = numberOfHops[j][caches[i].nodeKey] - numberOfHops[j][caches[i2].nodeKey];
						w[i2][j][0] = max(0, diff);
						caches[i2].totalOffer += w[i2][j][0] - temp;
					}
				}
			}

		}

		hasUnconnectedRequest = false;
		for (uint32_t i = 0; i < lenRequests; i++) {
			if (requests[i].cacheKey == -1) {
				hasUnconnectedRequest = true;
				break;
			}
		}
		printf("Time: %d\n", time);
	}

//	for (uint32_t i = 0; i < lenPairs; i++) {
//		clients[pairs[i].second].cacheKey = clients[pairs[i].first].cacheKey;
//	}
	return requests;
}

int minDistance(vector<int> dist, bool sptSet[], int numOfNodes) {
   // Initialize min value
   int min = INT_MAX, min_index;

   for (int v = 0; v < numOfNodes; v++)
     if (sptSet[v] == false && dist[v] <= min)
         min = dist[v], min_index = v;

   return min_index;
}

vector<vector<int>> dijkstra(vector<vector<int>> graph, int src, int numOfNodes) {
	vector<int> dist(numOfNodes);     // The output array.  dist[i] will hold the shortest
	vector<vector<int>> path(numOfNodes);
	// distance from src to i

	bool sptSet[numOfNodes]; // sptSet[i] will be true if vertex i is included in shortest
	// path tree or shortest distance from src to i is finalized

	// Initialize all distances as INFINITE and stpSet[] as false
	for (int i = 0; i < numOfNodes; i++) {
		dist[i] = INT_MAX, sptSet[i] = false;
		//path[i].push_back(-1);
	}

	// Distance of source vertex from itself is always 0
	dist[src] = 0;

	// Find shortest path for all vertices
	for (int count = 0; count < numOfNodes-1; count++) {
		// Pick the minimum distance vertex from the set of vertices not
		// yet processed. u is always equal to src in the first iteration.
		int u = minDistance(dist, sptSet, numOfNodes);

		// Mark the picked vertex as processed
		sptSet[u] = true;

		// Update dist value of the adjacent vertices of the picked vertex.
		for (int v = 0; v < numOfNodes; v++) {
			// Update dist[v] only if is not in sptSet, there is an edge from
			// u to v, and total weight of path from src to  v through u is
			// smaller than current value of dist[v]

			if (!sptSet[v] && graph[u][v] && dist[u] != INT_MAX && dist[u]+graph[u][v] < dist[v]) {
				dist[v] = dist[u] + graph[u][v];
				path[v] = path[u];
				path[v].push_back(u);
			}
		}
	}

	// print the constructed distance array
	return path;
}

void getCombinations(int offset, int k) {
	if (k == 0) {
		allCombinations.push_back(combination);
		return;
	}
	for (uint32_t i = offset; i <= caches.size() - k; i++) {
		combination.push_back(i);
		getCombinations(i+1, k-1);
		combination.pop_back();
	}
}

pair<vector<vector<int>>, vector<int>> getAllCosts(int numOfRequests, vector<vector<int>> c) {
	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		getCombinations(0, i+1);
	}
	vector<int> allCosts;
	vector<vector<int>> DVector;
	for (uint32_t i = 0; i < allCombinations.size(); i++) {
		vector<int> D (numOfRequests);
		fill(D.begin(), D.end(), -1);
		for (uint32_t j = 0; j < D.size(); j++) {
			for (uint32_t k = 0; k < caches.size(); k++) {
				for (uint32_t l = 0; l < allCombinations[i].size(); l++) {
					if (caches[k].nodeKey == allCombinations[i][l]) {
						if (D[j] == -1 || c[j][caches[k].nodeKey]< c[j][D[j]])
							D[j] = caches[k].nodeKey;
					}
				}
			}
		}

		int cost = 0;
		for (uint32_t j = 0; j < allCombinations[i].size(); j++) {
			cost += caches[j].initialCost;
		}
		for (uint32_t j = 0; j < D.size(); j++) {
			if (D[j] != -1) {
				cost += c[j][D[j]];
			}
		}
		if (cost > 0) {
			DVector.push_back(D);
			allCosts.push_back(cost);
		}
	}
	return pair<vector<vector<int>>, vector<int>> (DVector, allCosts);
}

int getRandomCost(uint32_t numOfRequests, uint32_t numOfCaches, vector<vector<int>> c) {
	set<int> openCaches;
	int cost = 0;
	while (openCaches.size() < numOfCaches) {
		openCaches.insert(rand() % NUM_OF_SERVERS);
	}
	Cache cache(-1);
	for (uint32_t i = 0; i < openCaches.size(); i++) {
		cost += cache.initialCost;
	}
	for (uint32_t i = 0; i < numOfRequests; i++) {
		int minCost = INT_MAX;
		set <int> :: iterator itr;
		for (itr = openCaches.begin(); itr != openCaches.end(); itr++) {
			minCost = min(minCost, c[i][*itr]);
		}
		cost += minCost;
	}
	return cost;
}

int getTotalCost(vector<Request> requests, vector<vector<int>> c) {
	int cost = 0;
	for (uint32_t i = 0; i < caches.size(); i++) {
		if (caches[i].isOpen)
			cost += caches[i].initialCost;
	}
	for (uint32_t i = 0; i < requests.size(); i++) {
		if (requests[i].cacheKey != -1) {
			cost += c[i][requests[i].cacheKey];
		}
	}
	return cost;
}

vector<vector<int>> createScaleFreeNetwork(uint32_t N, uint32_t m0) {
	uint32_t k_tot = 0; //d = 3;
	double a = 1.22;
	vector<int> G;
	vector<int> k (N);
	vector<vector<int>> adj (N, vector<int>(N));
	int numOfNodes = m0;

    //nodes.Create(m0);

    for (uint32_t i = 0; i < m0; i++) {
    	G.push_back(i);
    }

	for (uint32_t i = 0; i < m0; i++) {
		for (uint32_t j = i + 1; j < m0; j++) {
			//p2p.Install(nodes.Get(i), nodes.Get(j));
			adj[i][j] = 1;
			adj[j][i] = 1;
			k[i]++;
			k[j]++;
			k_tot += 2;
		}
	}

	while (G.size() < N) {
		//nodes.Add(node);
		numOfNodes++;
		uint32_t i = numOfNodes - 1;
		uint32_t numOfAdjs = 0;
		for (uint32_t k = 0; k < N; k++) {
			if (adj[i][k] == 1) {
				numOfAdjs++;
			}
		}
		while (numOfAdjs < m0) {
			uint32_t j = rand() % G.size();
			double P = pow(double(k[j]) / k_tot, a);
			double R = static_cast <double> (rand()) / static_cast <double> (RAND_MAX);
			if (P > R && (i < NUM_OF_SERVERS || j < NUM_OF_SERVERS)) {  // ??
				//p2p.Install(nodes.Get(i), nodes.Get(j));
				adj[i][j] = 1;
				adj[j][i] = 1;
				numOfAdjs++;
				k[i]++;
				k[j]++;
				k_tot += 2;
			}
		}
		G.push_back(i);
	}

	return adj;
}

vector<Request> getRandomRequests(uint32_t begin, uint32_t end) {
	vector<int> numbers(end-begin);
	for (uint32_t i = 0; i < end - begin; i++) {
		numbers[i] = i + begin;
	}
	random_shuffle(numbers.begin(), numbers.end());

	vector<Request> requests;
	for (uint32_t i = 0; i < numbers.size() - NUM_OF_REQUESTS_PER_CLIENT; i += 1) {
		for (uint32_t j = 0; j < NUM_OF_REQUESTS_PER_CLIENT; j++)
			requests.push_back(Request(numbers[i], numbers[i+j+1]));
	}

	return requests;
}

void updateNodeColors(bool turnOn) {
	if (turnOn) {
		for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
			if (caches[i].isOpen)
				anim->UpdateNodeImage(i, resourceOnlineCloudId);
		}
	}
	else {
		for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
			anim->UpdateNodeImage(i, resourceOfflineCloudId);
		}
	}
}

void updateNodeSizes(vector<int> args) {
	for (uint32_t i = 0; i < NUM_OF_NODES; i++) {
		anim->UpdateNodeSize(i, 20, 20);
	}
	for (uint32_t i = 0; i < args.size(); i++) {
		anim->UpdateNodeSize(args[i], 35, 35);
	}
}

int main(int argc, char *argv[]) {
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // create link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

    Ipv4AddressHelper ipv4;
	//Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(false));
    srand(time(NULL));

	vector<vector<int>> adjMatrix = createScaleFreeNetwork(NUM_OF_NODES, NUM_OF_SERVERS/4);
	NodeContainer nodes;
	nodes.Create(NUM_OF_NODES);

	vector<Request> requests = getRandomRequests(0, NUM_OF_NODES-NUM_OF_SERVERS);

	vector<vector<Ipv4InterfaceContainer>> interfaceVector(NUM_OF_NODES, vector<Ipv4InterfaceContainer>(NUM_OF_NODES));
    // create internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

	uint32_t ipCounter3 = 0, ipCounter2 = 0, ipCounter1 = 0, ipCounter0 = 0;
	for (uint32_t i = 0; i < NUM_OF_NODES; i++) {
		for (uint32_t j = 0; j < NUM_OF_NODES; j++) {
			if (adjMatrix[i][j] == 1) {
				NetDeviceContainer devices = p2p.Install(nodes.Get(i), nodes.Get(j));

				if (ipCounter1 < 255) {
					ipCounter1++;
				}
				else if (ipCounter2 < 255) {
					ipCounter2++;
					ipCounter1 = 0;
				}
				else {
					ipCounter3++;
					ipCounter2 = 0;
					ipCounter1 = 0;
				}
				ostringstream ipStream;
				ipStream << ipCounter3;
				ipStream << ".";
				ipStream << ipCounter2;
				ipStream << ".";
				ipStream << ipCounter1;
				ipStream << ".";
				ipStream << ipCounter0;

				ipv4.SetBase(ns3::Ipv4Address(ipStream.str().c_str()), "255.255.255.0");
				Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);
				interfaceVector[i][j] = interfaces;
			}
		}
	}

	int positions[NUM_OF_NODES][2];
	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		uint32_t rand_x = rand() % 1000 + 10;
		uint32_t rand_y = rand() % 500 + 10;
		positions[i][0] = rand_x;
		positions[i][1] = rand_y;

		caches.push_back(Cache(i));

		UdpEchoServerHelper echoServer(9999);
		ApplicationContainer serverApps = echoServer.Install(nodes.Get(i));
		serverApps.Start(Seconds(1.0));
		serverApps.Stop(Seconds(1000.0));
	}

	for (uint32_t i = NUM_OF_SERVERS; i < NUM_OF_NODES; i++) {
		uint32_t rand_x = rand() % 1000 + 10;
		uint32_t rand_y = rand() % 500 + 10;
		positions[i][0] = rand_x;
		positions[i][1] = rand_y;
	}

	vector<vector<int>> numberOfHops(requests.size(), vector<int>(NUM_OF_SERVERS));
	vector<vector<int>> rawNumberOfHops(requests.size(), vector<int>(NUM_OF_SERVERS));
	vector<int> numberOfHopsNoCache(requests.size());
	for (uint32_t i = 0; i < requests.size(); i++) {
		vector<vector<int>> distFrom = dijkstra(adjMatrix, requests[i].from+NUM_OF_SERVERS, NUM_OF_NODES);
		vector<vector<int>> distTo = dijkstra(adjMatrix, requests[i].to+NUM_OF_SERVERS, NUM_OF_NODES);
		for (uint32_t j = 0; j < NUM_OF_SERVERS; j++) {
			numberOfHops[i][j] = 2 * distFrom[j].size() + distTo[j].size();
			rawNumberOfHops[i][j] = distFrom[j].size();
		}
		numberOfHopsNoCache[i] = distFrom[requests[i].to+NUM_OF_SERVERS].size();
	}

	requests = simulate(requests, numberOfHops);
	int noCacheCost = accumulate(numberOfHopsNoCache.begin(), numberOfHopsNoCache.end(), 0);
	printf("No cache cost: %d\n", noCacheCost);
	int totalCost = getTotalCost(requests, rawNumberOfHops);
	printf("Optimized cost: %d\n", totalCost);
	int numOfOpenCaches = 0;
	for (uint32_t i = 0; i < caches.size(); i++) {
		if (caches[i].isOpen)
			numOfOpenCaches++;
	}
	int randCost = getRandomCost(requests.size(), NUM_OF_SERVERS/3, rawNumberOfHops);
	printf("Random cost: %d\n", randCost);
	printf("Number of open caches cost: %d\n", numOfOpenCaches);
//	pair<vector<vector<int>>, vector<int>> allCosts = getAllCosts(requests.size(), rawNumberOfHops);
//	sort(allCosts.second.begin(), allCosts.second.end());
//	printf("Best cost: %d\n", allCosts.second[0]);
//	int randIdx = rand() % allCosts.second.size();
//	printf("Random cost: %d\n", allCosts.second[randIdx]);
//	for (uint32_t i = 0; i < caches.size(); i++) {
//		caches[i].isOpen = false;
//	}
//	for (uint32_t i = 0; i < requests.size(); i++) {
//		requests[i].cacheKey = allCosts.first[randIdx][i];
//		caches[requests[i].cacheKey].isOpen = true;
//	}


	for (uint32_t i = 0; i < requests.size(); i++) {
		vector<int> pairVector (2);
		pairVector[0] = requests[i].from;
		pairVector[1] = requests[i].to;
		vector<int> args;
		args.push_back(pairVector[0]+NUM_OF_SERVERS);
		args.push_back(pairVector[1]+NUM_OF_SERVERS);
		Simulator::Schedule(Seconds(4 * i + 1), updateNodeColors, false);
		Simulator::Schedule(Seconds(4 * i + 1), updateNodeSizes, args);
		vector<vector<int>> path = dijkstra(adjMatrix, pairVector[0]+NUM_OF_SERVERS, NUM_OF_NODES);
		UdpEchoServerHelper echoServer(9);
		ApplicationContainer serverApps = echoServer.Install(nodes.Get(pairVector[1]+NUM_OF_SERVERS));
		serverApps.Start(Seconds(4 * i + 1));
		serverApps.Stop(Seconds(4 * i + 2));

		UdpEchoClientHelper echoClient(interfaceVector[path[pairVector[1]+NUM_OF_SERVERS].back()][pairVector[1]+NUM_OF_SERVERS].GetAddress(1), 9);
		echoClient.SetAttribute ("MaxPackets", UintegerValue(1));
		echoClient.SetAttribute ("Interval", TimeValue(Seconds (1)));
		echoClient.SetAttribute ("PacketSize", UintegerValue(1024));
		ApplicationContainer clientApps = echoClient.Install(nodes.Get(pairVector[0]+NUM_OF_SERVERS));
		clientApps.Start(Seconds(4 * i + 1));
		clientApps.Stop(Seconds(4 * i + 2));

		args.push_back(requests[i].cacheKey);
		Simulator::Schedule(Seconds(4 * i + 3), updateNodeColors, true);
		Simulator::Schedule(Seconds(4 * i + 3), updateNodeSizes, args);
		UdpEchoClientHelper echoClient2(interfaceVector[path[requests[i].cacheKey].back()][requests[i].cacheKey].GetAddress(1), 9999);
		echoClient2.SetAttribute ("MaxPackets", UintegerValue(1));
		echoClient2.SetAttribute ("Interval", TimeValue(Seconds (1)));
		echoClient2.SetAttribute ("PacketSize", UintegerValue(1024));
		ApplicationContainer clientApps2 = echoClient2.Install(nodes.Get(pairVector[0]+NUM_OF_SERVERS));
		clientApps2.Start(Seconds(4 * i + 3));
		clientApps2.Stop(Seconds(4 * i + 4));

	}

    anim = new AnimationInterface ("anim_test.xml");
	resourceOnlineCloudId = anim->AddResource ("/home/lenovo/source/ns-3.29/utils/icons/cloud_online.png");
	resourceOfflineCloudId = anim->AddResource ("/home/lenovo/source/ns-3.29/utils/icons/cloud_offline.png");
	resourceClientId = anim->AddResource ("/home/lenovo/source/ns-3.29/utils/icons/client.png");

	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		anim->SetConstantPosition(nodes.Get(i), positions[i][0], positions[i][1]);
		if (caches[i].isOpen)
			anim->UpdateNodeImage(i, resourceOnlineCloudId);
		else
			anim->UpdateNodeImage(i, resourceOfflineCloudId);
	}
	for (uint32_t i = NUM_OF_SERVERS; i < NUM_OF_NODES; i++) {
		anim->SetConstantPosition(nodes.Get(i), positions[i][0], positions[i][1]);
		anim->UpdateNodeImage(i, resourceClientId);
	}

// 	dump config
//  p2p.EnablePcapAll ("test");

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Run();
    Simulator::Destroy();
    delete anim;

    return 0;
}
