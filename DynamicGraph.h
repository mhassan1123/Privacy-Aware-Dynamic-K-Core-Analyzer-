#ifndef DYNAMICGRAPH_H
#define DYNAMICGRAPH_H

#include <string>
#include <vector>
#include <chrono>
#include "graph.h"
#include "KCore.h"

using namespace std;

struct Snapshot {
    string timestamp;
    size_t vertices;
    size_t edges;
    int core_2_size;
    int core_3_size;
    int core_5_size;
    int max_core;
    double runtime_ms;
};

class DynamicGraphAnalyzer {
private:
    Graph graph;
    KCoreAnalyzer kcore_analyzer;

public:
    DynamicGraphAnalyzer();

    Graph& getGraph() { return graph; }
    const Graph& getGraph() const { return graph; }
    const KCoreAnalyzer& getKCoreAnalyzer() const { return kcore_analyzer; }

    bool loadBaseGraph(const string& filename);
    vector<Snapshot> processUpdateStream(const string& filename, int batch_size = 2);
    void applyUpdate(const string& action, int u, int v);
    Snapshot captureSnapshot(const string& label);
    void printSnapshotReport(const vector<Snapshot>& snapshots) const;
    void exportResultsToCSV(const vector<Snapshot>& snapshots, const string& output_filepath) const;
};

#endif // DYNAMICGRAPH_H