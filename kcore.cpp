#include "KCore.h"
#include <iostream>
#include <algorithm>
#include <queue>

using namespace std;

unordered_map<int, int> KCoreAnalyzer::calculateCoreNumbers(const Graph& graph) const {
    unordered_map<int, int> core_numbers;
    unordered_map<int, int> degrees;
    queue<int> process_queue;

    vector<int> vertices = graph.getVertices();
    if (vertices.empty()) return core_numbers;

    // Step 1: Initialize vertex degrees
    for (int v : vertices) {
        degrees[v] = graph.degree(v);
    }

    // Step 2: Standard K-Core Peeling Algorithm
    int current_k = 0;
    int processed_count = 0;
    int total_vertices = static_cast<int>(vertices.size());

    while (processed_count < total_vertices) {
        // Enqueue all remaining vertices with current degree <= current_k
        for (int v : vertices) {
            if (core_numbers.find(v) == core_numbers.end() && degrees[v] <= current_k) {
                process_queue.push(v);
                core_numbers[v] = current_k; 
            }
        }

        // Peel vertices and update effective degrees of neighbors
        if (process_queue.empty()) {
            current_k++;
            continue;
        }

        while (!process_queue.empty()) {
            int u = process_queue.front();
            process_queue.pop();
            processed_count++;

            for (int neighbor : graph.getNeighbors(u)) {
                if (core_numbers.find(neighbor) == core_numbers.end()) {
                    degrees[neighbor]--;
                    // If neighbor's degree drops below or equal to current_k, enqueue and assign core
                    if (degrees[neighbor] <= current_k) {
                        process_queue.push(neighbor);
                        core_numbers[neighbor] = current_k;
                    }
                }
            }
        }
        current_k++;
    }

    return core_numbers;
}

vector<int> KCoreAnalyzer::getKCore(const Graph& graph, int k) const {
    unordered_map<int, int> core_numbers = calculateCoreNumbers(graph);
    vector<int> k_core_vertices;

    for (const auto& pair : core_numbers) {
        if (pair.second >= k) {
            k_core_vertices.push_back(pair.first);
        }
    }

    return k_core_vertices;
}

void KCoreAnalyzer::printReport(const Graph& graph) const {
    unordered_map<int, int> core_numbers = calculateCoreNumbers(graph);

    if (core_numbers.empty()) {
        cout << "Graph is empty.\n";
        return;
    }

    int max_core = 0;
    for (const auto& pair : core_numbers) {
        max_core = max(max_core, pair.second);
    }

    cout << "========== K-CORE ANALYSIS ==========\n\n";
    for (int k = 1; k <= max_core; ++k) {
        int count = 0;
        for (const auto& pair : core_numbers) {
            if (pair.second >= k) {
                count++;
            }
        }
        cout << k << "-Core: " << count << " vertices\n";
    }

    cout << "\nMaximum Core: " << max_core << "\n";
    cout << "=====================================\n";
}