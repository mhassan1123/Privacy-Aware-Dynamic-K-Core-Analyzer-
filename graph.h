#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;

class Graph {
private:
    unordered_map<int, vector<int>> adj;
    int num_edges;

public:
    Graph();

    // these functions will modify the graph, so NO const
    void addVertex(int v);
    void addEdge(int u, int v);
    void removeEdge(int u, int v);

    // Queries (read-only, ADD const)
    bool hasVertex(int v) const;
    bool hasEdge(int u, int v) const;
    int degree(int v) const;
    vector<int> getNeighbors(int v) const;

    // Graph metrics (read-only, ADD const)
    int vertexCount() const;
    int edgeCount() const;
    vector<int> getVertices() const;
};

#endif // GRAPH_H