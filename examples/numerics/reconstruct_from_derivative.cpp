// Selected reconstruction code from the private deformer.
// This sample demonstrates the simply-connected integration path used to
// recover vertex positions from a complex derivative field f'(z).
// Hole-period correction is intentionally omitted.

#include <Eigen/Dense>
#include <complex>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>
#include <limits>
#include <cmath>

using Cd    = std::complex<double>;
using VecCd = Eigen::VectorXcd;

static std::vector<std::pair<int, int>> buildUniqueEdges(
    const Eigen::MatrixXi& faces)
{
    std::set<std::pair<int, int>> edgeSet;

    for (int t = 0; t < faces.rows(); ++t) {
        int a = faces(t, 0);
        int b = faces(t, 1);
        int c = faces(t, 2);
        int e[3][2] = {{a, b}, {b, c}, {c, a}};

        for (const auto& edge : e) {
            int u = edge[0];
            int v = edge[1];
            if (u > v) std::swap(u, v);
            edgeSet.insert({u, v});
        }
    }

    return {edgeSet.begin(), edgeSet.end()};
}

static std::vector<std::vector<int>> buildAdjacency(
    int vertexCount,
    const std::vector<std::pair<int, int>>& edges)
{
    std::vector<std::vector<int>> adjacency(vertexCount);
    for (const auto& [u, v] : edges) {
        adjacency[u].push_back(v);
        adjacency[v].push_back(u);
    }
    return adjacency;
}

static std::vector<std::pair<int, int>> bfsSpanningTree(
    const std::vector<std::vector<int>>& adjacency,
    int root)
{
    std::vector<bool> visited(adjacency.size(), false);
    std::vector<std::pair<int, int>> tree;
    std::queue<int> q;

    visited[root] = true;
    q.push(root);

    while (!q.empty()) {
        const int parent = q.front();
        q.pop();

        for (int child : adjacency[parent]) {
            if (!visited[child]) {
                visited[child] = true;
                tree.push_back({parent, child});
                q.push(child);
            }
        }
    }

    return tree;
}

VecCd reconstructFromDerivative(
    const VecCd& z,
    const Eigen::MatrixXi& faces,
    const VecCd& fPrime,
    int anchor,
    Cd anchorValue)
{
    const int n = static_cast<int>(z.size());
    const auto edges = buildUniqueEdges(faces);
    const auto adjacency = buildAdjacency(n, edges);

    // Trapezoidal integration along every directed mesh edge:
    // df ~= 0.5 * (f'(u) + f'(v)) * (z(v) - z(u)).
    std::map<std::pair<int, int>, Cd> increment;
    for (const auto& [u, v] : edges) {
        const Cd df = 0.5 * (fPrime(u) + fPrime(v)) * (z(v) - z(u));
        increment[{u, v}] = df;
        increment[{v, u}] = -df;
    }

    VecCd f(n);
    f.setConstant(Cd(std::numeric_limits<double>::quiet_NaN(), 0.0));
    f(anchor) = anchorValue;

    for (const auto& [parent, child] : bfsSpanningTree(adjacency, anchor)) {
        f(child) = f(parent) + increment.at({parent, child});
    }

    // Handle disconnected mesh components independently.
    while (true) {
        int root = -1;
        for (int i = 0; i < n; ++i) {
            if (std::isnan(f(i).real())) {
                root = i;
                break;
            }
        }

        if (root < 0) break;

        f(root) = Cd(0.0, 0.0);
        for (const auto& [parent, child] : bfsSpanningTree(adjacency, root)) {
            f(child) = f(parent) + increment.at({parent, child});
        }
    }

    return f;
}
