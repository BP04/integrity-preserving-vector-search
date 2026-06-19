#include "test_large.h"
#include "VerifiableVectorDB.h"
#include "ClientVerifier.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cassert>

void run_large_test() {
    const int n = 1000000;
    const int dim = 16;
    const int m = 16;
    const int ef = 50;
    const int k = 10;
    const int num_queries = 1000;

    std::cout << "large benchmark with verification n=" << n << " dim=" << dim << " M=" << m << " ef=" << ef << "\n";

    VerifiableVectorDB db(m, ef);

    std::cout << "generating data\n";
    std::vector<std::vector<float>> dataset(n, std::vector<float>(dim));
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for(int i = 0; i < n; ++i) {
        for(int d = 0; d < dim; ++d) {
            dataset[i][d] = dist(rng);
        }
    }

    std::cout << "inserting\n";
    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < n; ++i) {
        db.insert(dataset[i]);
        if((i + 1) % 200000 == 0) {
            std::cout << "insert progress " << (i + 1) << "/" << n << "\n";
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double insert_sec = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "insert done " << insert_sec << "s\n";

    std::string root = db.get_root();

    std::vector<float> query(dim);
    for(int d = 0; d < dim; ++d) {
        query[d] = dist(rng);
    }
    db.query(query, k);

    std::cout << "query+verify k=" << k << " x" << num_queries << " (" << (num_queries * k) << " proofs)\n";

    double total_query_ms = 0.0;
    double total_verify_ms = 0.0;

    for(int i = 0; i < num_queries; ++i) {
        query[0] += 0.001f;

        auto q0 = std::chrono::high_resolution_clock::now();
        auto results = db.query(query, k);
        auto q1 = std::chrono::high_resolution_clock::now();

        for(const auto& r : results) {
            bool ok = verify_merkle_proof(r.vector_data, r.id, r.merkle_proof, root);
            assert(ok);
        }
        auto q2 = std::chrono::high_resolution_clock::now();

        total_query_ms += std::chrono::duration<double, std::milli>(q1 - q0).count();
        total_verify_ms += std::chrono::duration<double, std::milli>(q2 - q1).count();
    }

    double avg_query_ms = total_query_ms / num_queries;
    double avg_verify_ms = total_verify_ms / num_queries;
    double avg_total_ms = avg_query_ms + avg_verify_ms;

    std::cout << "query avg " << avg_query_ms << "ms verify avg " << avg_verify_ms << "ms total " << avg_total_ms << "ms " << (1000.0 / avg_total_ms) << " qps\n";
    std::cout << "large test passed\n";
}
