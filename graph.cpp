#include "graph.h"
using namespace std;

Graph::Graph() : num_edges(0) {}

void Graph::addVertex(int v) {
    if (adj.find(v) == adj.end()) {
        adj[v] = vector<int>();
    }
}

void Graph::addEdge(int u, int v){
    addVertex(u);
    addVertex(v);

    // Prevent self-loops and duplicate edges
    if (u != v && !hasEdge(u, v)) {
        adj[u].push_back(v);
        adj[v].push_back(u);
        num_edges++;
    }
}

void Graph::removeEdge(int u, int v) {
    if (hasEdge(u, v)) {
        // Remove v from u's vector
        auto& u_neighbors = adj[u];
        u_neighbors.erase(remove(u_neighbors.begin(), u_neighbors.end(), v), u_neighbors.end());

        // Remove u from v's vector
        auto& v_neighbors = adj[v];
        v_neighbors.erase(remove(v_neighbors.begin(), v_neighbors.end(), u), v_neighbors.end());

        num_edges--;
    }
}

bool Graph::hasVertex(int v)const{
    return adj.find(v) != adj.end();
}

bool Graph::hasEdge(int u, int v)const{
    auto it = adj.find(u);
    if (it != adj.end()) {
        const auto& neighbors = it->second;
        return find(neighbors.begin(), neighbors.end(), v) != neighbors.end();
    }
    return false;
}

int Graph::degree(int v)const{
    auto it = adj.find(v);
    if (it != adj.end()) {
        return static_cast<int>(it->second.size());
    }
    return 0;
}

vector<int> Graph::getNeighbors(int v)const{
    auto it = adj.find(v);
    if (it != adj.end()) {
        return it->second;
    }
    return {};
}

int Graph::vertexCount()const{
    return adj.size();
}

int Graph::edgeCount()const{
    return num_edges;
}

vector<int> Graph::getVertices()const{
    vector<int> vertices;
    vertices.reserve(adj.size());
    for (const auto& pair : adj) {
        vertices.push_back(pair.first);
    }
    return vertices;
}