import itertools
import random
import cProfile


def create_random_tuples(num_of_caches, num_of_clients):
    """
    Creates caches and clients with random opening costs and random connection costs
    :param num_of_caches: total number of caches
    :param num_of_clients: number of clients
    :return:
    """

    F = [[0, int(random.random() * 25) + 5, 0] for _ in range(0, num_of_caches)]
    c = [[int(random.random() * 15) + 2 for _ in range(0, num_of_clients)] for _ in range(0, num_of_caches)]

    return F, c


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
    F, c = create_random_tuples(num_of_caches, num_of_clients)

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
    #
    # return final_cost, all_costs, ratio
    return final_cost, D, num_of_placed_caches, distribution_of_caches


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

    simulate(100, 5000)

    pr.disable()
    pr.print_stats(sort='time')
