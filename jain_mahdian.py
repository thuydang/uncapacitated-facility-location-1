import itertools
import random
import cProfile
import math
import plotly.plotly as py
from plotly.graph_objs import *


def create_random_tuples(num_of_caches, num_of_clients):
    """
    Creates caches and clients with random opening costs and random connection costs
    :param num_of_caches: total number of caches
    :param num_of_clients: number of clients
    :return:
    """

    F = [[0, 25, 0] for _ in range(0, num_of_caches)]
    c = [[int(random.random() * 15) + 2 for _ in range(0, num_of_clients)] for _ in range(0, num_of_caches)]

    return F, c


def create_random_tuples_with_coordinates(num_of_caches, num_of_clients):
    F = [[0, 500, 0, int(random.random() * 100), int(random.random() * 100)] for _ in range(0, num_of_caches)]
    clients = [[int(random.random() * 100), int(random.random() * 100)] for _ in range(0, num_of_clients)]
    c = [[int(math.sqrt((cache[3] - client[0]) ** 2 + (cache[4] - client[1]) ** 2)) for client in clients] for cache in F]

    return F, clients, c


def plot(F, D, clients):
    trace1_x = []
    trace1_y = []
    trace2_x = []
    trace2_y = []
    colors = []

    for cache in F:
        trace2_x.append(cache[3])
        trace2_y.append(cache[4])
        colors.append(0)
    for idx, connection in enumerate(D):
        trace2_x.append(clients[idx][0])
        trace2_y.append(clients[idx][1])
        colors.append(10)
        trace1_x.append(clients[idx][0])
        trace1_y.append(clients[idx][1])
        trace1_x.append(F[connection][3])
        trace1_y.append(F[connection][4])
        trace1_x.append(None)
        trace1_y.append(None)

    py.sign_in('username', 'api_key')  # plotly credentials
    trace1 = {
        "x": trace1_x,
        "y": trace1_y,
        "hoverinfo": "none",
        "line": {
            "color": "#888",
            "width": 0.5
        },
        "mode": "lines",
        "type": "scatter"
    }
    trace2 = {
        "x": trace2_x,
        "y": trace2_y,
        "hoverinfo": "text",
        "marker": {
            "color": colors,
            "colorbar": {
                "thickness": 15,
                "title": "Node Connections",
                "titleside": "right",
                "xanchor": "left"
            },
            "colorscale": [[0, 'white'], [1.0, 'rgb(0, 0, 255)']],
            "line": {"width": 2},
            "reversescale": True,
            "showscale": True,
            "size": 8
        },
        "mode": "markers",
        "text": ["# of connections: 9", "# of connections: 9", "# of connections: 13", "# of connections: 4",
                 "# of connections: 2", "# of connections: 10", "# of connections: 11", "# of connections: 9"],
        "type": "scatter"
    }
    data = Data([trace1, trace2])
    layout = {
        "annotations": [
            {
                "x": 0.005,
                "y": -0.002,
                "showarrow": False,
                "text": "Python code: <a href='https://plot.ly/ipython-notebooks/network-graphs/'> https://plot.ly/ipython-notebooks/network-graphs/</a>",
                "xref": "paper",
                "yref": "paper"
            }
        ],
        "height": 650,
        "hovermode": "closest",
        "margin": {
            "r": 5,
            "t": 40,
            "b": 20,
            "l": 5
        },
        "showlegend": False,
        "title": "<br>Network graph made with Python",
        "titlefont": {"size": 16},
        "width": 650,
        "xaxis": {
            "showgrid": False,
            "showticklabels": False,
            "zeroline": False
        },
        "yaxis": {
            "showgrid": False,
            "showticklabels": False,
            "zeroline": False
        }
    }
    fig = Figure(data=data, layout=layout)
    plot_url = py.plot(fig)


def total_cost(F, c, D):
    """
    Returns total cost (i.e sum of cache opening costs and connection costs)
    :param F: list of cache lists as follows: [cache_0 = [status (0 -> closed, 1 -> open), opening cost, total_offer, ...], ...]
    :param c: connection costs as follows: [cache_0 costs = [costs for each client], ...]
    :param D: clients list as follows: [client_0_connected_cache_id, client_1_connected_cache_id, ...]
    :return: total cost
    """

    cost = sum([x[1] for x in F if x[0] == 1])
    for idx, value in enumerate(D):
        if value is not None:
            cost += c[value][idx]
    return cost


def simulate(num_of_caches, num_of_clients):
    """
    Creates caches and clients with random opening costs and random connection costs, and estimates which caches to
    be opened for minimum cost
    :param num_of_caches: total number of caches
    :param num_of_clients: number of clients
    :return final_cost: final sum of cache opening costs and connection costs
    :return D: clients list as follows: [client_0_connected_cache_id, client_1_connected_cache_id, ...]
    :return num_of_placed_caches: number of opened caches
    :return distribution_of_caches: distribution of caches by the number of clients connected to them
    """

    D = [None for _ in range(0, num_of_clients)]
    v = [0 for _ in range(0, num_of_clients)]
    w = [[[0, 0] for _ in range(0, num_of_clients)] for _ in range(0, num_of_caches)]
    F, clients, c = create_random_tuples_with_coordinates(num_of_caches, num_of_clients)

    has_unconnected_client = None in D
    time = 0

    while has_unconnected_client:
        time += 1
        for j in range(0, len(D)):
            if D[j] is None:
                v[j] += 1
            for i in range(0, len(F)):
                if F[i][0] == 0 and D[j] is None and c[i][j] == v[j]:
                    w[i][j][1] = 1
                if D[j] is None and w[i][j][1] == 1:
                    w[i][j][0] += 1
                    F[i][2] += 1
                if F[i][0] == 0 and F[i][2] == F[i][1]:
                    F[i][0] = 1
                    for j2 in range(0, len(D)):
                        if w[i][j2][0] > 0:
                            D[j2] = i
                            for i2 in range(0, len(F)):
                                temp = w[i2][j2][0]
                                w[i2][j2][0] = max(0, c[i][j2] - c[i2][j2])
                                F[i2][2] += w[i2][j2][0] - temp
                if F[i][0] == 1 and D[j] is None and c[i][j] == v[j]:
                    D[j] = i
                    for i2 in range(0, len(F)):
                        temp = w[i2][j][0]
                        w[i2][j][0] = max(0, c[i][j] - c[i2][j])
                        F[i2][2] += w[i2][j][0] - temp

        has_unconnected_client = None in D
        print("Time :", time)

    final_cost = total_cost(F, c, D)
    print("Total cost: {0}".format(final_cost))
    print("Connections: {0}".format(D))
    num_of_placed_caches = sum([1 for x in F if x[0] == 1])
    print("Number of placed caches: {0} out of {1}".format(num_of_placed_caches, num_of_caches))
    distribution_of_caches = sorted(freq(D).items(), key=lambda x: x[1], reverse=True)
    print("Distribution of caches by the number of clients: {0}".format(distribution_of_caches))

    # all_costs = evaluate_all_costs(num_of_clients, c, F)
    # print("All costs: {0}".format(all_costs))
    # ratio = final_cost / all_costs[0][0]
    # print("Total cost / best cost: {0}".format(ratio))

    print("Caches: {0}, Clients: {1}".format(F, clients))
    plot(F, D, clients)
    # return final_cost, all_costs, ratio
    # return final_cost, D, num_of_placed_caches, distribution_of_caches


def freq(lst):
    """
    Computes frequency of list elements
    :param lst: a list of numbers
    :return d: a dictionary as follows: {element: number_of_occurences_in_list}
    """
    d = {}
    for i in lst:
        if d.get(i):
            d[i] += 1
        else:
            d[i] = 1
    return d


def evaluate_all_costs(num_of_clients, c, F):
    """
    Evaluates costs for all possible cache combinations
    :param num_of_clients: number of clients
    :param c: connection costs as follows: [cache_0 costs = [costs for each client], ...]
    :param F: list of cache lists as follows: [cache_0 = [status (0 -> closed, 1 -> open), opening cost, total_offer, ...], ...]
    :return all_costs: list of sorted cost tuples as follows: [[cost, D (list_of_clients_connected_cache_ids)]]
    """

    facilities = range(0, len(F))
    subsets = []
    all_costs = []
    for L in range(0, len(facilities) + 1):
        for subset in itertools.combinations(facilities, L):
            subsets.append(subset)

    for subset in subsets:
        D2 = [None for _ in range(0, num_of_clients)]
        for idx, value in enumerate(D2):
            for idx2, value2 in enumerate(F):
                if idx2 in subset:
                    if D2[idx] is None:
                        D2[idx] = idx2
                    elif c[idx2][idx] < c[D2[idx]][idx]:
                        D2[idx] = idx2
        cost = 0
        for s in subset:
            cost += F[s][1]
        for idx, value in enumerate(D2):
            if value is not None:
                cost += c[value][idx]
        if cost > 0:
            all_costs.append([cost, D2])

    all_costs.sort(key=lambda x: x[0])
    return all_costs


if __name__ == '__main__':
    pr = cProfile.Profile()
    pr.enable()

    simulate(50, 500)

    pr.disable()
    pr.print_stats(sort='time')
