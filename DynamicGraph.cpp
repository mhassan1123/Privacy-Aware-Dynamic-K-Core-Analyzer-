#include "DynamicGraph.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

using namespace std;

DynamicGraphAnalyzer::DynamicGraphAnalyzer() {}

bool DynamicGraphAnalyzer::loadBaseGraph(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open base graph file: " << filename << "\n";
        return false;
    }

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        // Support both comma-separated and space-separated formats
        for (char &c : line) {
            if (c == ','){ 
                c = ' ';
            }
        }

        stringstream ss(line);
        int u, v;
        if (ss >> u >> v) {
            graph.addEdge(u, v);
        }
    }
    file.close();
    return true;
}

void DynamicGraphAnalyzer::applyUpdate(const string& action, int u, int v) {
    if (action == "ADD") {
        graph.addEdge(u, v);
    } else if (action == "REMOVE") {
        graph.removeEdge(u, v);
    } else {
        cerr << "Warning: Unknown update action '" << action << "'\n";
    }
}

Snapshot DynamicGraphAnalyzer::captureSnapshot(const string& label) {
    auto start_time = chrono::high_resolution_clock::now();

    unordered_map<int, int> core_numbers = kcore_analyzer.calculateCoreNumbers(graph);

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed = end_time - start_time;

    int c3_size = 0;
    int c5_size = 0;
    int max_core = 0;

    for (const auto& pair : core_numbers) {
        if (pair.second >= 3) c3_size++;
        if (pair.second >= 5) c5_size++;
        max_core = max(max_core, pair.second);
    }

    Snapshot snap;
    snap.timestamp = label;
    snap.vertices = graph.vertexCount();
    snap.edges = graph.edgeCount();
    snap.core_3_size = c3_size;
    snap.core_5_size = c5_size;
    snap.max_core = max_core;
    snap.runtime_ms = elapsed.count();

    return snap;
}

vector<Snapshot> DynamicGraphAnalyzer::processUpdateStream(const string& filename, int batch_size) {
    vector<Snapshot> snapshots;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: Could not open updates file: " << filename << "\n";
        return snapshots;
    }

    snapshots.push_back(captureSnapshot("T0 (Base)"));

    string line;
    int update_count = 0;
    int snapshot_idx = 1;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        // Support both CSV and space-delimited updates
        for (char &c : line) {
            if (c == ',') c = ' ';
        }

        stringstream ss(line);
        string action;
        int u, v;

        if (ss >> action >> u >> v) {
            applyUpdate(action, u, v);
            update_count++;

            if (update_count % batch_size == 0) {
                string label = "T" + to_string(snapshot_idx++);
                snapshots.push_back(captureSnapshot(label));
            }
        }
    }

    if (update_count % batch_size != 0) {
        string label = "T" + to_string(snapshot_idx);
        snapshots.push_back(captureSnapshot(label));
    }

    file.close();
    return snapshots;
}

void DynamicGraphAnalyzer::printSnapshotReport(const vector<Snapshot>& snapshots) const {
    cout << "========================================================================\n";
    cout << "                    DYNAMIC GRAPH SNAPSHOT REPORT                       \n";
    cout << "========================================================================\n";
    cout << left 
         << setw(12) << "Time" 
         << setw(12) << "Vertices" 
         << setw(10) << "Edges" 
         << setw(12) << "3-Core" 
         << setw(12) << "5-Core" 
         << setw(12) << "Max Core" 
         << setw(12) << "Time (ms)" << "\n";
    cout << "------------------------------------------------------------------------\n";

    for (const auto& s : snapshots) {
        cout << left 
             << setw(12) << s.timestamp 
             << setw(12) << s.vertices 
             << setw(10) << s.edges 
             << setw(12) << s.core_3_size 
             << setw(12) << s.core_5_size 
             << setw(12) << s.max_core 
             << fixed << setprecision(3) << setw(12) << s.runtime_ms << "\n";
    }
    cout << "========================================================================\n\n";
}

void DynamicGraphAnalyzer::exportResultsToCSV(const vector<Snapshot>& snapshots, const string& output_filepath) const {
    ofstream file(output_filepath);
    if (!file.is_open()) {
        cerr << "Error: Could not open output file for export: " << output_filepath << "\n";
        return;
    }

    file << "Time,Vertices,Edges,3-Core,5-Core,MaxCore,Runtime_ms\n";
    for (const auto& s : snapshots) {
        file << s.timestamp << ","
             << s.vertices << ","
             << s.edges << ","
             << s.core_3_size << ","
             << s.core_5_size << ","
             << s.max_core << ","
             << fixed << setprecision(3) << s.runtime_ms << "\n";
    }
    file.close();
    cout << "Results exported successfully to: " << output_filepath << "\n";
}