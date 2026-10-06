// server.cpp - exposes your existing C++ classes over HTTP so the web page can use them.
// It does NOT change any of your files. It only #includes them and calls their functions.
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include "httplib.h"          // the tiny web-server library (one file)
#include "graph.h"            // your files, unchanged
#include "KCore.h"
#include "DynamicGraph.h"
#include "PrivacyManager.h"

using namespace std;

static mutex mtx;                                                    // one request at a time
static unique_ptr<DynamicGraphAnalyzer> analyzer(new DynamicGraphAnalyzer());
static PrivacyManager privacy;
static vector<Snapshot> snapshots;

static const string EDGES_TMP   = "data/_uploaded_edges.csv";
static const string UPDATES_TMP = "data/_uploaded_updates.csv";

// ---- small helpers to build JSON text by hand ----
static string esc(const string& s) {
    string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (c == '\n') o += "\\n";
        else if (c == '\r') {}
        else o += c;
    }
    return o;
}
static string num(double d) {
    ostringstream os; os << fixed << setprecision(4) << d; return os.str();
}
static int countCore(const unordered_map<int, int>& cores, int k) {
    int n = 0;
    for (const auto& p : cores) if (p.second >= k) n++;
    return n;
}
static void fail(httplib::Response& res, const string& m) {
    res.status = 400;
    res.set_content(m, "text/plain");
}
static bool writeFile(const string& path, const string& text) {
    ofstream f(path);
    if (!f.is_open()) return false;
    f << text;
    return true;
}

// Current graph + core numbers, computed by YOUR KCoreAnalyzer
static string stateJson() {
    const Graph& g = analyzer->getGraph();
    auto t0 = chrono::high_resolution_clock::now();
    unordered_map<int, int> cores = analyzer->getKCoreAnalyzer().calculateCoreNumbers(g);
    double ms = chrono::duration<double, milli>(chrono::high_resolution_clock::now() - t0).count();

    vector<int> ids = g.getVertices();
    sort(ids.begin(), ids.end());
    int mx = 0;
    for (const auto& p : cores) mx = max(mx, p.second);

    ostringstream o;
    o << "{\"V\":" << g.vertexCount() << ",\"E\":" << g.edgeCount()
      << ",\"max\":" << mx << ",\"c3\":" << countCore(cores, 3) << ",\"c5\":" << countCore(cores, 5)
      << ",\"ms\":" << num(ms) << ",\"vertices\":[";
    for (size_t i = 0; i < ids.size(); i++) {
        o << (i ? "," : "") << "{\"id\":" << ids[i] << ",\"deg\":" << g.degree(ids[i])
          << ",\"core\":" << cores[ids[i]] << "}";
    }
    o << "],\"edges\":[";
    bool first = true;
    for (int u : ids)
        for (int v : g.getNeighbors(u))
            if (u < v) { o << (first ? "" : ",") << "[" << u << "," << v << "]"; first = false; }
    o << "]}";
    return o.str();
}
static string snapsJson() {
    ostringstream o; o << "[";
    for (size_t i = 0; i < snapshots.size(); i++) {
        const Snapshot& s = snapshots[i];
        o << (i ? "," : "") << "{\"label\":\"" << esc(s.timestamp) << "\",\"V\":" << s.vertices
          << ",\"E\":" << s.edges << ",\"c3\":" << s.core_3_size << ",\"c5\":" << s.core_5_size
          << ",\"mx\":" << s.max_core << ",\"ms\":" << num(s.runtime_ms) << "}";
    }
    o << "]";
    return o.str();
}

int main() {
    httplib::Server svr;
    svr.set_mount_point("/", "./web");   // serves web/index.html at http://localhost:8080

    // GET /api/state -> graph, core numbers, stats
    svr.Get("/api/state", [](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        res.set_content(stateJson(), "application/json");
    });

    // POST /api/load  (body = edge list text)  -> menu option 1
    svr.Post("/api/load", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        if (!writeFile(EDGES_TMP, req.body)) return fail(res, "Cannot write data/ folder. Run the server from the project folder.");
        analyzer.reset(new DynamicGraphAnalyzer());                  // fresh, empty analyzer
        if (!analyzer->loadBaseGraph(EDGES_TMP) || analyzer->getGraph().vertexCount() == 0)
            return fail(res, "Failed to load graph (no valid edges found).");
        snapshots.clear();
        snapshots.push_back(analyzer->captureSnapshot("T0 (Base)"));
        cout << "[server] graph loaded: " << analyzer->getGraph().vertexCount() << " vertices, "
             << analyzer->getGraph().edgeCount() << " edges\n";
        res.set_content(stateJson(), "application/json");
    });

    // POST /api/edge?action=ADD|REMOVE&u=1&v=2  -> menu options 4 and 5
    svr.Post("/api/edge", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string action = req.get_param_value("action");
        if ((action != "ADD" && action != "REMOVE") || !req.has_param("u") || !req.has_param("v"))
            return fail(res, "Invalid input.");
        if (analyzer->getGraph().vertexCount() == 0) return fail(res, "Graph is empty! Please load network first.");
        try {
            analyzer->applyUpdate(action, stoi(req.get_param_value("u")), stoi(req.get_param_value("v")));
        } catch (...) { return fail(res, "Invalid input."); }
        res.set_content(stateJson(), "application/json");
    });

    // POST /api/stream?batch=2 (body = update lines) -> menu option 6
    svr.Post("/api/stream", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        if (analyzer->getGraph().vertexCount() == 0) return fail(res, "Graph is empty! Please load network first.");
        int batch = 2;
        try { batch = max(1, stoi(req.get_param_value("batch"))); } catch (...) {}
        if (!writeFile(UPDATES_TMP, req.body)) return fail(res, "Cannot write data/ folder.");
        snapshots = analyzer->processUpdateStream(UPDATES_TMP, batch);
        analyzer->printSnapshotReport(snapshots);                    // also prints in this console window
        res.set_content("{\"snaps\":" + snapsJson() + ",\"state\":" + stateJson() + "}", "application/json");
    });

    // GET /api/export -> menu option 9 (writes results/dynamic_results.csv with YOUR function)
    svr.Get("/api/export", [](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        if (snapshots.empty()) {
            if (analyzer->getGraph().vertexCount() == 0) return fail(res, "Nothing to export.");
            snapshots.push_back(analyzer->captureSnapshot("Current State"));
        }
        analyzer->exportResultsToCSV(snapshots, "results/dynamic_results.csv");
        string csv = "Time,Vertices,Edges,3-Core,5-Core,MaxCore,Runtime_ms\n";
        for (const Snapshot& s : snapshots)
            csv += s.timestamp + "," + to_string(s.vertices) + "," + to_string(s.edges) + "," +
                   to_string(s.core_3_size) + "," + to_string(s.core_5_size) + "," +
                   to_string(s.max_core) + "," + num(s.runtime_ms) + "\n";
        res.set_content("{\"snaps\":" + snapsJson() + ",\"csv\":\"" + esc(csv) + "\"}", "application/json");
    });

    // GET /api/privacy?eps=1 -> menu option 7
    svr.Get("/api/privacy", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        double eps = 0;
        try { eps = stod(req.get_param_value("eps")); } catch (...) {}
        if (eps <= 0) return fail(res, "Invalid epsilon value.");
        if (analyzer->getGraph().vertexCount() == 0) return fail(res, "Graph is empty! Please load network first.");

        const int N = 10000;
        double scale = 1.0 / eps, sum = 0, mn = numeric_limits<double>::infinity(), mxv = -mn;
        vector<double> xs; xs.reserve(N);
        for (int i = 0; i < N; i++) {
            double n = privacy.laplaceNoise(scale);
            xs.push_back(n); sum += n; mn = min(mn, n); mxv = max(mxv, n);
        }
        double mean = sum / N, var = 0;
        for (double x : xs) var += (x - mean) * (x - mean);
        var /= N;

        auto cores = analyzer->getKCoreAnalyzer().calculateCoreNumbers(analyzer->getGraph());
        double exact = countCore(cores, 3);
        double priv = privacy.privatize(exact, 1.0, eps);
        ostringstream o;
        o << "{\"b\":" << num(scale) << ",\"mean\":" << num(mean) << ",\"var\":" << num(var)
          << ",\"theoVar\":" << num(2 * scale * scale) << ",\"min\":" << num(mn) << ",\"max\":" << num(mxv)
          << ",\"exact\":" << exact << ",\"private\":" << num(priv) << ",\"error\":" << num(fabs(priv - exact)) << "}";
        res.set_content(o.str(), "application/json");
    });

    // GET /api/compare -> menu option 8
    svr.Get("/api/compare", [](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        if (analyzer->getGraph().vertexCount() == 0) return fail(res, "Graph is empty! Please load network first.");
        auto cores = analyzer->getKCoreAnalyzer().calculateCoreNumbers(analyzer->getGraph());
        double exact = countCore(cores, 3);
        ostringstream o; o << "{\"exact\":" << exact << ",\"rows\":[";
        double eps[] = {0.5, 1.0, 2.0, 5.0};
        for (int i = 0; i < 4; i++) {
            double p = privacy.privatize(exact, 1.0, eps[i]);
            o << (i ? "," : "") << "{\"eps\":" << eps[i] << ",\"private\":" << num(p) << ",\"error\":" << num(fabs(p - exact)) << "}";
        }
        o << "]}";
        res.set_content(o.str(), "application/json");
    });

    cout << "K-Core server running.  Open  http://localhost:8080  in your browser.\n"
         << "(Keep this window open. Press Ctrl+C to stop.)\n";
    if (!svr.listen("127.0.0.1", 8080)) {
        cerr << "Could not start server (is port 8080 already in use?)\n";
        return 1;
    }
    return 0;
}