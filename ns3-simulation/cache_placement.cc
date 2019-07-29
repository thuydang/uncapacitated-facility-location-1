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

NS_LOG_COMPONENT_DEFINE ("FirstExample");
const int NUM_OF_NODES = 100;
const int NUM_OF_REQUESTS_PER_CLIENT = 2;
const int NUM_OF_GRIDS = 8;
const int NUM_OF_SERVERS = 10;

std::vector<int> combination;
std::vector<std::vector<int>> allCombinations;
AnimationInterface* anim = 0;
uint32_t resourceOnlineCloudId;
uint32_t resourceOfflineCloudId;
uint32_t resourceClientId;
PointToPointHelper p2p;
Ipv4AddressHelper ipv4;
NodeContainer nodes;

struct NumOfHops{
	std::vector<std::vector<int>> biasedNumOfHops;  // 2 * num of hops from requesting client to the cache
													// + num of hops from responding client to the cache
	std::vector<std::vector<int>> unbiasedNumOfHops;  // num of hops from requesting client to the cache
													// + num of hops from responding client to the cache
	std::vector<int> numOfHopsNoCache;  // num of hops from requesting client to the responding client
};

class Request {
public:
	int from, to;  // from requesting client idx to the responding client idx
	int cacheKey;  // assigned cache key for the request, -1 for no assignment

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
	int nodeKey;  // node key that cache is stored
	int initialCost;  // can be changed to determine the number of placed caches
	int totalOffer;
	bool isOpen;

	Cache(int nodeKey) {
		this->nodeKey = nodeKey;
		initialCost = 10;
		totalOffer = 0;
		isOpen = false;
	}
};
std::vector<Cache> caches;  // global cache vector

class Position {
public:
	int x, y;  // x and y coordinates
	int key;  // node key

	Position(int key, int x, int y) {
		this->key = key;
		this->x = x;
		this->y = y;
	}
};

std::vector<std::vector<int>> createScaleFreeNetwork(uint32_t N, uint32_t m0) {
	/*
	 * Create scale free network
	 * input N: total number of nodes
	 * input m0: initial number of nodes that will be connected to each other
	 * return adj: adjacency matrix of the network
	 */

	uint32_t k_tot = 0, d = 1;  // d is the minimum number of degrees required for each node
	double a = 1.22;
	uint32_t curNumOfNodes = 0;
	std::vector<int> k (N);
	std::vector<std::vector<int>> adj (N, std::vector<int>(N));
	int numOfNodes = m0;

    for (uint32_t i = 0; i < m0; i++) {
    	curNumOfNodes++;
    }

	for (uint32_t i = 0; i < m0; i++) {
		for (uint32_t j = i + 1; j < m0; j++) {
			adj[i][j] = 1;
			adj[j][i] = 1;
			k[i]++;
			k[j]++;
			k_tot += 2;
		}
	}

	while (curNumOfNodes < N) {
		numOfNodes++;
		uint32_t i = numOfNodes - 1;
		uint32_t numOfAdjs = 0;
		for (uint32_t k = 0; k < N; k++) {
			if (adj[i][k] == 1) {
				numOfAdjs++;
			}
		}
		while (numOfAdjs < d) {
			uint32_t j = rand() % curNumOfNodes;
			double P = pow(double(k[j]) / k_tot, a);
			double R = static_cast <double> (rand()) / static_cast <double> (RAND_MAX);
			if (P > R && (i < NUM_OF_SERVERS || j < NUM_OF_SERVERS)) {
				adj[i][j] = 1;
				adj[j][i] = 1;
				numOfAdjs++;
				k[i]++;
				k[j]++;
				k_tot += 2;
			}
		}
		curNumOfNodes++;
	}

	return adj;
}

std::vector<Request> optimize(std::vector<Request> requests, std::vector<std::vector<int>> numOfHops) {
	/*
	 * Optimize cache placement and assignments
	 * input requests: request pattern vector
	 * input numOfHops: number of hops vector for each request such as:
	 * [
	 * 		request_0:
	 * 			[num_of_hops between request_0 and cache_0,
	 * 			num_of_hops between request_0 and cache_1,
	 * 			...],
	 * 		request_1:
	 * 			...
	 * ...]
	 * return requests: request vector with cache assignments
	 */

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
	// run the algorithm until there is no unconnected request exists
	while (hasUnconnectedRequest) {
		time++;
		for (uint32_t j = 0; j < lenRequests; j++) {
			if (requests[j].cacheKey == -1) {
				v[j]++;
			}
			for (uint32_t i = 0; i < lenCaches; i++) {
				if (!caches[i].isOpen && requests[j].cacheKey == -1
						&& numOfHops[j][caches[i].nodeKey] == v[j]) {
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
								int diff = numOfHops[j2][caches[i].nodeKey]
										- numOfHops[j2][caches[i2].nodeKey];
								w[i2][j2][0] = std::max(0, diff);
								caches[i2].totalOffer += w[i2][j2][0] - temp;
							}
						}
					}
				}
				if (caches[i].isOpen && requests[j].cacheKey == -1
						&& numOfHops[j][caches[i].nodeKey] == v[j]) {
					requests[j].cacheKey = caches[i].nodeKey;
					for (uint32_t i2 = 0; i2 < lenCaches; i2++) {
						int temp = w[i2][j][0];
						int diff = numOfHops[j][caches[i].nodeKey]
								- numOfHops[j][caches[i2].nodeKey];
						w[i2][j][0] = std::max(0, diff);
						caches[i2].totalOffer += w[i2][j][0] - temp;
					}
				}
			}
		}

		// check if unconnected request exists
		hasUnconnectedRequest = false;
		for (uint32_t i = 0; i < lenRequests; i++) {
			if (requests[i].cacheKey == -1) {
				hasUnconnectedRequest = true;
				break;
			}
		}
	}

	return requests;
}

int minDistance(std::vector<int> dist, bool sptSet[], int numOfNodes) {
   // Initialize min value
   int min = INT_MAX, min_index;

   for (int v = 0; v < numOfNodes; v++)
     if (sptSet[v] == false && dist[v] <= min)
         min = dist[v], min_index = v;

   return min_index;
}

std::vector<std::vector<int>> dijkstra(std::vector<std::vector<int>> adjMatrix, int src, int numOfNodes) {
	/*
	 * Dijkstra's Shortest Path algorithm
	 * input adjMatrix: adjacency matrix
	 * input src: starting node idx
	 * input numOfNodes: total number of nodes
	 * return path: shortest path vector from src to each node
	 */

	std::vector<int> dist(numOfNodes);     // The output array.  dist[i] will hold the shortest
	std::vector<std::vector<int>> path(numOfNodes);
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

			if (!sptSet[v] && adjMatrix[u][v] && dist[u] != INT_MAX && dist[u]+adjMatrix[u][v] < dist[v]) {
				dist[v] = dist[u] + adjMatrix[u][v];
				path[v] = path[u];
				path[v].push_back(u);
			}
		}
	}

	// print the constructed distance array
	return path;
}

int getRandomCost(uint32_t numOfRequests, uint32_t numOfCaches, std::vector<std::vector<int>> c) {
	/*
	 * Get random cache assignment cost
	 * input numOfRequests : number of requests
	 * input numOfCaches: number of caches to be opened
	 * input c: number of hops vector
	 * return cost: cost of the random cache assignment
	 */

	std::set<int> openCaches;
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
		std::set <int> :: iterator itr;
		for (itr = openCaches.begin(); itr != openCaches.end(); itr++) {
			minCost = std::min(minCost, c[i][*itr]);
		}
		cost += minCost;
	}
	return cost;
}

int getTotalCost(std::vector<Request> requests, std::vector<std::vector<int>> c) {
	/*
	 * Get the total cost based on the given assignments
	 * input requests: request vector with cache assignments
	 * input c: number of hops vector
	 * return cost: total cost of the given assignments
	 */

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

std::vector<Request> getRandomRequests(std::vector<std::vector<Position>> pMap) {
	/*
	 * Create random request patterns between the clients from the same grid with 80% probability,
	 * and from the different grid with 20% probability
	 * input pMap: position map such as [grid_0: requests in the grid_0, grid_1: requests in the grid_1, ...]
	 * return requests: request patterns vector
	 */

	std::vector<Request> requests;
	for (uint32_t i = 0; i < pMap.size(); i++) {
		if (pMap[i].size() > 0) {
			int randFrom = rand() % pMap[i].size();
			for (uint32_t j = 0; j < 10; j++) {
				if (pMap[i][randFrom].key < NUM_OF_SERVERS)
					randFrom = rand() % pMap[i].size();
				else
					break;
			}
			if (pMap[i][randFrom].key < NUM_OF_SERVERS)
				continue;

			int numOfRequests = 0;
			while (numOfRequests < 3) {
				double prob = static_cast <double> (rand()) / static_cast <double> (RAND_MAX);
				if (prob < 0.8) {
					int randTo = rand() % pMap[i].size();
					for (uint32_t j = 0; j < 10; j++) {
						if (randTo == randFrom || pMap[i][randTo].key < NUM_OF_SERVERS)
							randTo = rand() % pMap[i].size();
						else
							break;
					}
					if (randTo != randFrom && pMap[i][randTo].key >= NUM_OF_SERVERS) {
						requests.push_back(Request(pMap[i][randFrom].key, pMap[i][randTo].key));
					}
					numOfRequests++;
				}
				else {
					int randTo = rand() % pMap[NUM_OF_GRIDS-1-i].size();
					for (uint32_t j = 0; j < 10; j++) {
						if (pMap[NUM_OF_GRIDS-1-i][randTo].key < NUM_OF_SERVERS)
							randTo = rand() % pMap[NUM_OF_GRIDS-1-i].size();
						else
							break;
					}
					if (pMap[NUM_OF_GRIDS-1-i][randTo].key >= NUM_OF_SERVERS) {
						requests.push_back(Request(pMap[i][randFrom].key, pMap[NUM_OF_GRIDS-1-i][randTo].key));
					}
					numOfRequests++;
				}
			}
		}
	}

	return requests;
}

void updateNodeColors(bool turnOn) {
	/*
	 * Update node colors while simulating
	 */

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

void updateNodeSizes(std::vector<int> args) {
	/*
	 * Update node sizes while simulating
	 */

	for (uint32_t i = 0; i < NUM_OF_NODES; i++) {
		anim->UpdateNodeSize(i, 20, 20);
	}
	for (uint32_t i = 0; i < args.size(); i++) {
		anim->UpdateNodeSize(args[i], 35, 35);
	}
}

std::vector<std::vector<Ipv4InterfaceContainer>> createInterfaceVector(NodeContainer nodes, std::vector<std::vector<int>> adjMatrix) {
	std::vector<std::vector<Ipv4InterfaceContainer>> interfaceVector(NUM_OF_NODES, std::vector<Ipv4InterfaceContainer>(NUM_OF_NODES));
	/*
	 * Create p2p links and interface vector between the given nodes if they are adjacent
	 * input nodes: all nodes container
	 * input adjMatrix: adjacency matrix of the network
	 * return interfaceVector: interface vector between each node
	 */

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
				std::ostringstream ipStream;
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
	return interfaceVector;
}

int evaluateGrid(int x, int y) {
	/*
	 * Evaluates the grid based on the x and y coordinates
	 */

	int xDiv = x / 250;
	int yDiv = y / 250;
	return xDiv + 4*yDiv;
}

std::vector<std::vector<Position>> createRandomPositions() {
	/*
	 * Create random position for each node
	 * return pMap: position map such as [grid_0: requests in the grid_0, grid_1: requests in the grid_1, ...]
	 */

	std::vector<std::vector<Position>> pMap (NUM_OF_GRIDS);
	for (uint32_t i = 0; i < NUM_OF_NODES; i++) {
		uint32_t randX = rand() % 1000;
		uint32_t randY = rand() % 500;
		int grid = evaluateGrid(randX, randY);
		pMap[grid].push_back(Position(i, randX, randY));
	}
	return pMap;
}

NumOfHops evaluateNumOfHops(std::vector<Request> requests, std::vector<std::vector<int>> adjMatrix) {
	/*
	 * Evaluate number of hops for each request using dijkstra's shortest path
	 * input requests: request patterns
	 * input adjMatrix: adjacency matrix of the network
	 * return numOfHops: NumOfHops struct
	 */

	NumOfHops numOfHops;
	std::vector<std::vector<int>> biasedNumOfHops(requests.size(), std::vector<int>(NUM_OF_SERVERS));
	std::vector<std::vector<int>> unbiasedNumOfHops(requests.size(), std::vector<int>(NUM_OF_SERVERS));
	std::vector<int> numOfHopsNoCache(requests.size());
	for (uint32_t i = 0; i < requests.size(); i++) {
		std::vector<std::vector<int>> distFrom = dijkstra(adjMatrix, requests[i].from, NUM_OF_NODES);
		std::vector<std::vector<int>> distTo = dijkstra(adjMatrix, requests[i].to, NUM_OF_NODES);
		for (uint32_t j = 0; j < NUM_OF_SERVERS; j++) {
			biasedNumOfHops[i][j] = 2 * distFrom[j].size() + distTo[j].size();
			unbiasedNumOfHops[i][j] = distFrom[j].size();
		}
		numOfHopsNoCache[i] = distFrom[requests[i].to].size();
	}
	numOfHops.biasedNumOfHops = biasedNumOfHops;
	numOfHops.unbiasedNumOfHops = unbiasedNumOfHops;
	numOfHops.numOfHopsNoCache = numOfHopsNoCache;
	return numOfHops;
}

void simulate(std::vector<Request> requests, std::vector<std::vector<int>> adjMatrix,
		std::vector<std::vector<Ipv4InterfaceContainer>> interfaceVector) {
	/*
	 * Run the ns3 simulation
	 * input requests: request patterns
	 * input adjMatrix: adjacency matrix of the network
	 * input interfaceVector: interface vector
	 */

	for (uint32_t i = 0; i < requests.size(); i++) {
		std::vector<int> pairVector (2);
		pairVector[0] = requests[i].from;
		pairVector[1] = requests[i].to;
		std::vector<std::vector<int>> path = dijkstra(adjMatrix, pairVector[0], NUM_OF_NODES);
		std::vector<int> args;

		args.push_back(pairVector[0]);
		args.push_back(pairVector[1]);
		Simulator::Schedule(Seconds(4 * i + 1), updateNodeColors, false);
		Simulator::Schedule(Seconds(4 * i + 1), updateNodeSizes, args);
		UdpEchoServerHelper echoServer(9);
		ApplicationContainer serverApps = echoServer.Install(nodes.Get(pairVector[1]));
		serverApps.Start(Seconds(4 * i + 1));
		serverApps.Stop(Seconds(4 * i + 2));
		UdpEchoClientHelper echoClient(interfaceVector[path[pairVector[1]].back()][pairVector[1]].GetAddress(1), 9);
		echoClient.SetAttribute ("MaxPackets", UintegerValue(1));
		echoClient.SetAttribute ("Interval", TimeValue(Seconds (1)));
		echoClient.SetAttribute ("PacketSize", UintegerValue(1024));
		ApplicationContainer clientApps = echoClient.Install(nodes.Get(pairVector[0]));
		clientApps.Start(Seconds(4 * i + 1));
		clientApps.Stop(Seconds(4 * i + 2));

		args.push_back(requests[i].cacheKey);
		Simulator::Schedule(Seconds(4 * i + 3), updateNodeColors, true);
		Simulator::Schedule(Seconds(4 * i + 3), updateNodeSizes, args);
		UdpEchoServerHelper echoServer2(10);
		ApplicationContainer serverApps2 = echoServer2.Install(nodes.Get(requests[i].cacheKey));
		serverApps2.Start(Seconds(4 * i + 1));
		serverApps2.Stop(Seconds(4 * i + 2));
		UdpEchoClientHelper echoClient2(interfaceVector[path[requests[i].cacheKey].back()][requests[i].cacheKey].GetAddress(1), 10);
		echoClient2.SetAttribute ("MaxPackets", UintegerValue(1));
		echoClient2.SetAttribute ("Interval", TimeValue(Seconds (1)));
		echoClient2.SetAttribute ("PacketSize", UintegerValue(1024));
		ApplicationContainer clientApps2 = echoClient2.Install(nodes.Get(pairVector[0]));
		clientApps2.Start(Seconds(4 * i + 3));
		clientApps2.Stop(Seconds(4 * i + 4));
	}
}

int main(int argc, char *argv[]) {
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

//	Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(false));
    srand(100);

    // create the scale free network
	std::vector<std::vector<int>> adjMatrix = createScaleFreeNetwork(NUM_OF_NODES, 2);
	// create the nodes
	nodes.Create(NUM_OF_NODES);

	// create internet stack
	InternetStackHelper internet;
	internet.Install(nodes);
	// crete random positions for each node
	std::vector<std::vector<Position>> positionMap = createRandomPositions();

	// create random request patterns
	std::vector<Request> requests = getRandomRequests(positionMap);
	std::vector<std::vector<Ipv4InterfaceContainer>> interfaceVector = createInterfaceVector(nodes, adjMatrix);

	// push all possible caches
	for (uint32_t i = 0; i < NUM_OF_SERVERS; i++) {
		caches.push_back(Cache(i));
	}

	NumOfHops numOfHops = evaluateNumOfHops(requests, adjMatrix);
	// run the optimization algorithm and get the requests with assignments
	requests = optimize(requests, numOfHops.biasedNumOfHops);

	int numOfOpenCaches = 0;
	for (uint32_t i = 0; i < caches.size(); i++) {
		if (caches[i].isOpen)
			numOfOpenCaches++;
	}

	// cost of the optimized placement
	int totalCost = getTotalCost(requests, numOfHops.biasedNumOfHops);
	printf("Optimized cost: %d\n", totalCost);
	// cost of the random placement
	int randCost = getRandomCost(requests.size(), NUM_OF_SERVERS/3, numOfHops.biasedNumOfHops);
	printf("Random cost: %d\n", randCost);
	printf("Number of open caches cost: %d\n", numOfOpenCaches);

	// run the ns3 simulation
	simulate(requests, adjMatrix, interfaceVector);

	// create animation trace file for netanim
    anim = new AnimationInterface ("anim_test.xml");
	resourceOnlineCloudId = anim->AddResource ("/home/lenovo/source/ns-3.29/utils/icons/cloud_online.png");
	resourceOfflineCloudId = anim->AddResource ("/home/lenovo/source/ns-3.29/utils/icons/cloud_offline.png");
	resourceClientId = anim->AddResource ("/home/lenovo/source/ns-3.29/utils/icons/client.png");

	// set the positions and images in netanim
	for (uint32_t i = 0; i < positionMap.size(); i++) {
		for (uint32_t j = 0; j < positionMap[i].size(); j++) {
			int nodeIdx = positionMap[i][j].key;
			anim->SetConstantPosition(nodes.Get(nodeIdx), positionMap[i][j].x, positionMap[i][j].y);
			if (nodeIdx < NUM_OF_SERVERS) {
				anim->UpdateNodeImage(nodeIdx, resourceOfflineCloudId);
			}
			else {
				anim->UpdateNodeImage(nodeIdx, resourceClientId);
			}
		}
	}

// 	dump config
//  p2p.EnablePcapAll ("test");

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Run();
    Simulator::Destroy();
    delete anim;

    return 0;
}
