#include "test_small.h"
#include "VerifiableVectorDB.h"
#include "ClientVerifier.h"
#include <iostream>
#include <vector>
#include <cassert>

static void print_vec(const std::vector<float>& v) {
    std::cout << "[";
    for(size_t i = 0; i < v.size(); ++i) {
        std::cout << v[i];
        if(i + 1 < v.size()) {
            std::cout << ", ";
        }
    }
    std::cout << "]";
}

void run_small_test() {
    std::cout << "small test with verification\n";

    VerifiableVectorDB db(16, 50);

    std::vector<std::vector<float>> dataset = {
        {0.10, 0.20, 0.30},
        {0.90, 0.80, 0.70},
        {0.15, 0.25, 0.35},
        {0.50, 0.50, 0.50},
        {0.95, 0.85, 0.75}
    };

    for(size_t i = 0; i < dataset.size(); ++i) {
        int id = db.insert(dataset[i]);
        std::cout << "insert id=" << id << " vec=";
        print_vec(dataset[i]);
        std::cout << "\n";
    }

    std::string root = db.get_root();
    std::cout << "root=" << root << "\n";

    std::vector<float> query = {0.12, 0.22, 0.32};
    int k = 2;
    std::cout << "query k=" << k << "\n";

    auto results = db.query(query, k);
    for(const auto& r : results) {
        std::cout << "hit id=" << r.id << " vec=";
        print_vec(r.vector_data);
        std::cout << "\n";
        bool ok = verify_merkle_proof(r.vector_data, r.id, r.merkle_proof, root);
        assert(ok);
        std::cout << "verified\n";
    }

    std::cout << "small test passed\n";
}
