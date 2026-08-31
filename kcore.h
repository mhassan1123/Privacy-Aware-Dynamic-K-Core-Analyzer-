#ifndef KCORE_H
#define KCORE_H

#include <vector>
#include <unordered_map>
#include "graph.h"

using namespace std;

class KCoreAnalyzer {
public:
    // Returns a list of vertex IDs belonging to the k-core (vertices with core number >= k)
    vector<int> getKCore(const Graph& graph, int k) const;

    // Computes and returns the core number for every vertex in the graph
    unordered_map<int, int> calculateCoreNumbers(const Graph& graph) const;

    // Prints a summary report of k-core sizes and maximum core number
    void printReport(const Graph& graph) const;
};

#endif // KCORE_H