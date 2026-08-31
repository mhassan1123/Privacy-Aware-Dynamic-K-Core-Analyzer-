#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <limits>
#include <fstream>
#include "graph.h"
#include "KCore.h"
#include "DynamicGraph.h"
#include "PrivacyManager.h"

using namespace std;

// Helper function to count nodes with core number >= k
int getCoreCount(const unordered_map<int, int>& core_numbers, int k) {
    int count = 0;
    for (const auto& pair : core_numbers) {
        if (pair.second >= k) {
            count++;
        }
    }
    return count;
}

// Step 4: Validate noise generator (10,000 samples)
void validateNoiseGenerator(PrivacyManager& privacy, double sensitivity, double epsilon, int num_samples = 10000) {
    double scale = sensitivity / epsilon;
    vector<double> samples;
    samples.reserve(num_samples);

    double sum = 0.0;
    double min_val = numeric_limits<double>::infinity();
    double max_val = -numeric_limits<double>::infinity();

    for (int i = 0; i < num_samples; ++i) {
        double noise = privacy.laplaceNoise(scale);
        samples.push_back(noise);
        sum += noise;
        if (noise < min_val) min_val = noise;
        if (noise > max_val) max_val = noise;
    }

    double mean = sum / num_samples;
    double variance_sum = 0.0;
    for (double val : samples) {
        variance_sum += (val - mean) * (val - mean);
    }
    double variance = variance_sum / num_samples;

    double expected_mean = 0.0;
    double expected_variance = 2.0 * scale * scale;

    cout << "========================================================================\n";
    cout << " STEP 4: LAPLACE NOISE GENERATOR VALIDATION (" << num_samples << " Samples)\n";
    cout << " Sensitivity = " << sensitivity << ", Epsilon = " << epsilon << " (Scale b = " << scale << ")\n";
    cout << "========================================================================\n";
    cout << left << setw(18) << "Metric" 
         << setw(18) << "Empirical" 
         << setw(18) << "Theoretical" << "\n";
    cout << "------------------------------------------------------------------------\n";
    cout << fixed << setprecision(4);
    cout << setw(18) << "Mean" << setw(18) << mean << setw(18) << expected_mean << "\n";
    cout << setw(18) << "Variance" << setw(18) << variance << setw(18) << expected_variance << "\n";
    cout << setw(18) << "Minimum" << setw(18) << min_val << setw(18) << "-Inf" << "\n";
    cout << setw(18) << "Maximum" << setw(18) << max_val << setw(18) << "+Inf" << "\n";
    cout << "========================================================================\n\n";
}

int main() {
    DynamicGraphAnalyzer analyzer;
    PrivacyManager privacy;
    vector<Snapshot> snapshots;
    int choice = -1;

    while (choice != 0) {
        cout << "========================================\n";
        cout << " Privacy-Aware Dynamic K-Core Analyzer\n";
        cout << "========================================\n";
        cout << "1. Load Network\n";
        cout << "2. Network Statistics\n";
        cout << "3. K-Core Analysis\n";
        cout << "4. Add Edge\n";
        cout << "5. Remove Edge\n";
        cout << "6. Process Update File\n";
        cout << "7. Privacy Analysis\n";
        cout << "8. Compare Exact vs Private\n";
        cout << "9. Export Results\n";
        cout << "0. Exit\n";
        cout << "----------------------------------------\n";
        cout << "Enter choice: ";
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "\n[Error]: Invalid choice. Please enter a number.\n\n";
            continue;
        }

        switch (choice) {
            case 1: { // Load Network
                string filepath = "data/sample_edges.csv";
                cout << "Loading base graph topology from " << filepath << "...\n";
                if (analyzer.loadBaseGraph(filepath)) {
                    snapshots.clear();
                    snapshots.push_back(analyzer.captureSnapshot("T0 (Base)"));
                    cout << "Graph loaded successfully! (" 
                         << analyzer.getGraph().vertexCount() << " vertices, " 
                         << analyzer.getGraph().edgeCount() << " edges)\n\n";
                } else {
                    cout << "[Error]: Failed to load graph.\n\n";
                }
                break;
            }
            case 2: { // Network Statistics
                const Graph& g = analyzer.getGraph();
                cout << "\n========== NETWORK STATISTICS ==========\n";
                cout << "Vertices: " << g.vertexCount() << "\n";
                cout << "Edges   : " << g.edgeCount() << "\n";
                cout << "========================================\n\n";
                break;
            }
            case 3: { // K-Core Analysis
                if (analyzer.getGraph().vertexCount() == 0) {
                    cout << "\n[Error]: Graph is empty! Please load network first.\n\n";
                } else {
                    analyzer.getKCoreAnalyzer().printReport(analyzer.getGraph());
                    cout << "\n";
                }
                break;
            }
            case 4: { // Add Edge
                int u, v;
                cout << "Enter edge (u v): ";
                if (cin >> u >> v) {
                    analyzer.applyUpdate("ADD", u, v);
                    cout << "Added edge (" << u << ", " << v << ") successfully.\n\n";
                } else {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "[Error]: Invalid input.\n\n";
                }
                break;
            }
            case 5: { // Remove Edge
                int u, v;
                cout << "Enter edge (u v): ";
                if (cin >> u >> v) {
                    analyzer.applyUpdate("REMOVE", u, v);
                    cout << "Removed edge (" << u << ", " << v << ") successfully.\n\n";
                } else {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "[Error]: Invalid input.\n\n";
                }
                break;
            }
            case 6: { // Process Update File
                string filepath = "data/updates.csv";
                int batch_size = 2;
                cout << "Processing stream from " << filepath << " (batch size = " << batch_size << ")...\n\n";
                snapshots = analyzer.processUpdateStream(filepath, batch_size);
                analyzer.printSnapshotReport(snapshots);
                break;
            }
            case 7: { // Privacy Analysis
                if (analyzer.getGraph().vertexCount() == 0) {
                    cout << "\n[Error]: Graph is empty! Please load network or process updates first.\n\n";
                    break;
                }

                double epsilon;
                cout << "epsilon = ";
                if (!(cin >> epsilon) || epsilon <= 0) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "[Error]: Invalid epsilon value.\n\n";
                    break;
                }

                // Step 4 Validation Run
                validateNoiseGenerator(privacy, 1.0, epsilon, 10000);

                // Analyze 3-Core Target Metric
                unordered_map<int, int> cores = analyzer.getKCoreAnalyzer().calculateCoreNumbers(analyzer.getGraph());
                double exact_val = static_cast<double>(getCoreCount(cores, 3));
                double private_val = privacy.privatize(exact_val, 1.0, epsilon);
                double error = abs(private_val - exact_val);

                cout << fixed << setprecision(1);
                cout << "Exact (3-Core Size):\n" << static_cast<int>(exact_val) << "\n\n";
                cout << "Private:\n" << private_val << "\n\n";
                cout << "Error:\n" << error << "\n\n";
                break;
            }
            case 8: { // Compare Exact vs Private
                if (analyzer.getGraph().vertexCount() == 0) {
                    cout << "\n[Error]: Graph is empty! Please load network or process updates first.\n\n";
                    break;
                }

                unordered_map<int, int> cores = analyzer.getKCoreAnalyzer().calculateCoreNumbers(analyzer.getGraph());
                double exact_val = static_cast<double>(getCoreCount(cores, 3));
                vector<double> epsilons = {0.5, 1.0, 2.0, 5.0};

                cout << "\nTarget Metric: 3-Core Size (Exact Value = " << static_cast<int>(exact_val) << ")\n";
                cout << left << setw(12) << "epsilon" 
                     << setw(12) << "exact" 
                     << setw(14) << "private" 
                     << setw(12) << "error" << "\n";
                cout << "-----------------------------------------------\n";

                for (double eps : epsilons) {
                    double privatized = privacy.privatize(exact_val, 1.0, eps);
                    double error = abs(privatized - exact_val);
                    cout << fixed << setprecision(1)
                         << setw(12) << eps 
                         << setw(12) << static_cast<int>(exact_val) 
                         << setw(14) << privatized 
                         << setw(12) << error << "\n";
                }
                cout << "-----------------------------------------------\n\n";
                break;
            }
            case 9: { // Export Results
                if (snapshots.empty()) {
                    cout << "\n[Warning]: No stream snapshots available. Exporting current graph state...\n";
                    snapshots.push_back(analyzer.captureSnapshot("Current State"));
                }
                analyzer.exportResultsToCSV(snapshots, "results/dynamic_results.csv");
                cout << "\n";
                break;
            }
            case 0: { // Exit
                cout << "\nExiting Dynamic K-Core Graph Analyzer. Goodbye!\n";
                break;
            }
            default: {
                cout << "\n[Error]: Invalid choice. Please select an option from 0 to 9.\n\n";
                break;
            }
        }
    }

    return 0;
}