#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <vector>

using Candidate = std::pair<float, int32_t>;

using MaxHeap = std::priority_queue<Candidate>;
using MinHeap = std::priority_queue<Candidate,
                                    std::vector<Candidate>,
                                    std::greater<Candidate>>;

struct VisitedTable {
    std::vector<uint16_t> visited;
    uint16_t current_generation = 0;

    void advance(int32_t node_count) {
        if(static_cast<int32_t>(visited.size()) < node_count) {
            visited.resize(node_count, 0);
        }
        current_generation++;
        if(current_generation == 0) {
            std::fill(visited.begin(), visited.end(), 0);
            current_generation = 1;
        }
    }

    bool is_visited(int32_t id) const {
        return visited[id] == current_generation;
    }
    void set_visited(int32_t id) {
        visited[id] = current_generation;
    }
};

template <int Dim = 0>
class HNSW {
public:
    explicit HNSW(int M = 16, int M0 = 0, int ef_construct = 200, int dim = 0)
        : M_(M),
          M0_(M0 > 0 ? M0 : 2 * M),
          ef_construct_(ef_construct),
          dim_(Dim > 0 ? Dim : dim),
          entry_point_(-1),
          max_layer_(-1),
          node_count_(0),
          rng_(std::random_device{}()),
          level_mult_(1.0 / std::log(static_cast<double>(M))) {
        if(dim_ <= 0) {
            throw std::invalid_argument("HNSW: dimensionality must be > 0");
        }
        if(M_ < 2) {
            throw std::invalid_argument("HNSW: M must be >= 2");
        }
    }

    HNSW(const HNSW&) = delete;
    HNSW& operator=(const HNSW&) = delete;
    HNSW(HNSW&&) = default;
    HNSW& operator=(HNSW&&) = default;

    int32_t size() const {
        return static_cast<int32_t>(node_count_);
    }
    int max_layer() const {
        return max_layer_;
    }
    int dim() const {
        return dim_;
    }

    const float* get_vector(int32_t id) const {
        return vectors_.data() + static_cast<size_t>(id) * dim_;
    }

    int32_t insert(const float* q);
    std::vector<Candidate> search_knn(const float* q, int K, int ef_search = 0) const;

private:
    struct AdjLayer {
        std::vector<int32_t> data;
        std::vector<int32_t> degree;
        int32_t M_max;
        int32_t capacity;

        AdjLayer() : M_max(0), capacity(0) {}
        explicit AdjLayer(int32_t m) : M_max(m), capacity(0) {}

        void reserve_nodes(int32_t n) {
            if(n <= capacity) {
                return;
            }
            data.resize(static_cast<size_t>(n) * M_max, -1);
            degree.resize(n, 0);
            capacity = n;
        }

        int32_t* neighbors(int32_t node) {
            return data.data() + static_cast<size_t>(node) * M_max;
        }
        const int32_t* neighbors(int32_t node) const {
            return data.data() + static_cast<size_t>(node) * M_max;
        }
        int32_t deg(int32_t node) const {
            return degree[node];
        }
    };

    int M_, M0_, ef_construct_, dim_;
    int32_t entry_point_;
    int max_layer_;
    int32_t node_count_;

    std::vector<float> vectors_;
    std::vector<int> node_layer_;
    std::vector<AdjLayer> layers_;

    mutable std::mt19937 rng_;
    double level_mult_;

    static constexpr size_t NUM_LOCKS = 65536;
    std::mutex locks_[NUM_LOCKS];

    inline std::mutex& get_node_lock(int32_t id) {
        return locks_[id % NUM_LOCKS];
    }

    inline float distance(const float* q, int32_t node) const {
        const float* v = get_vector(node);
        float sum = 0.0f;

        if constexpr(Dim > 0) {
            #if defined(__GNUC__) || defined(__clang__)
            #pragma GCC unroll 8
            #endif
            for(int d = 0; d < Dim; ++d) {
                float diff = q[d] - v[d];
                sum += diff * diff;
            }
        } else {
            for(int d = 0; d < dim_; ++d) {
                float diff = q[d] - v[d];
                sum += diff * diff;
            }
        }
        return sum;
    }

    inline float distance(int32_t u, int32_t v_node) const {
        return distance(get_vector(u), v_node);
    }

    int sample_layer();
    int32_t alloc_node(const float* q, int top_layer);

    int32_t search_layer_ef1(const float* q, int32_t ep, int lc) const;
    MaxHeap search_layer(const float* q, const std::vector<int32_t>& ep, int ef, int lc) const;

    std::vector<int32_t> select_neighbors_simple(MaxHeap W, int M) const;
    std::vector<int32_t> select_neighbors_heuristic(const float* q, MaxHeap W, int M, int lc, bool extend_candidates = true, bool keep_pruned = true) const;

    void connect_neighbors(int32_t node, const std::vector<int32_t>& neighbors, int lc);
    void shrink_neighbors(int32_t u, int32_t new_nb, int lc);
};
