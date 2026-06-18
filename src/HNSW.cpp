#include "HNSW.h"
#include <cmath>
#include <algorithm>

SimpleHNSW::SimpleHNSW(int M, int ef_construct)
    : M(M), ef_construct(ef_construct), entry_point(-1), max_layer(-1), rng(42) {
    level_mult = 1.0 / std::log(M);
}

float SimpleHNSW::distance(const std::vector<float>& a, const std::vector<float>& b) const {
    float sum = 0;
    for(size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

int SimpleHNSW::sample_layer() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return static_cast<int>(-std::log(dist(rng)) * level_mult);
}

int SimpleHNSW::search_layer_ef1(const std::vector<float>& q, int ep, int lc) {
    int best_node = ep;
    float best_dist = distance(q, nodes[ep].vec);
    bool changed = true;

    while(changed) {
        changed = false;
        for(int nbr : nodes[best_node].neighbors[lc]) {
            float d = distance(q, nodes[nbr].vec);
            if(d < best_dist) {
                best_dist = d;
                best_node = nbr;
                changed = true;
            }
        }
    }
    return best_node;
}

MaxHeap SimpleHNSW::search_layer(const std::vector<float>& q, int ep, int ef, int lc) {
    std::vector<bool> visited(nodes.size(), false);
    MaxHeap W;
    MinHeap C;

    float dist_ep = distance(q, nodes[ep].vec);
    W.push({dist_ep, ep});
    C.push({dist_ep, ep});
    visited[ep] = true;

    while(!C.empty()) {
        auto [d_c, c] = C.top();
        C.pop();

        if(d_c > W.top().first) {
            break;
        }

        for(int nbr : nodes[c].neighbors[lc]) {
            if(!visited[nbr]) {
                visited[nbr] = true;
                float d_e = distance(q, nodes[nbr].vec);
                if(W.size() < static_cast<size_t>(ef) || d_e < W.top().first) {
                    C.push({d_e, nbr});
                    W.push({d_e, nbr});
                    if(W.size() > static_cast<size_t>(ef)) {
                        W.pop();
                    }
                }
            }
        }
    }
    return W;
}

int SimpleHNSW::insert(const std::vector<float>& q) {
    int id = nodes.size();
    int top_layer = sample_layer();

    Node new_node;
    new_node.vec = q;
    new_node.neighbors.resize(top_layer + 1);
    nodes.push_back(new_node);

    if(id == 0) {
        entry_point = 0;
        max_layer = top_layer;
        return id;
    }

    int ep = entry_point;
    for(int lc = max_layer; lc > top_layer; --lc) {
        ep = search_layer_ef1(q, ep, lc);
    }

    for(int lc = std::min(top_layer, max_layer); lc >= 0; --lc) {
        MaxHeap W = search_layer(q, ep, ef_construct, lc);

        std::vector<int> selected_neighbors;
        while(!W.empty() && selected_neighbors.size() < static_cast<size_t>(M)) {
            selected_neighbors.push_back(W.top().second);
            W.pop();
        }

        nodes[id].neighbors[lc] = selected_neighbors;
        for(int nbr : selected_neighbors) {
            nodes[nbr].neighbors[lc].push_back(id);
        }

        if(!selected_neighbors.empty()) {
            ep = selected_neighbors[0];
        }
    }

    if(top_layer > max_layer) {
        max_layer = top_layer;
        entry_point = id;
    }
    return id;
}

std::vector<Candidate> SimpleHNSW::search(const std::vector<float>& q, int K) {
    if(entry_point == -1) {
        return {};
    }

    int ep = entry_point;
    for(int lc = max_layer; lc > 0; --lc) {
        ep = search_layer_ef1(q, ep, lc);
    }

    MaxHeap W = search_layer(q, ep, std::max(ef_construct, K), 0);

    std::vector<Candidate> res;
    while(!W.empty()) {
        res.push_back(W.top());
        W.pop();
    }
    std::reverse(res.begin(), res.end());
    if(res.size() > static_cast<size_t>(K)) {
        res.resize(K);
    }
    return res;
}

const std::vector<float>& SimpleHNSW::get_vector(int id) const {
    return nodes[id].vec;
}
