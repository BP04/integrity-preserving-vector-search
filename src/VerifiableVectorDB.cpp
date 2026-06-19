#include "VerifiableVectorDB.h"

VerifiableVectorDB::VerifiableVectorDB(int M, int ef_construct)
    : index(M, ef_construct) {}

int VerifiableVectorDB::insert(const std::vector<float>& vec) {
    int hnsw_id = index.insert(vec);
    merkle_tree.append_leaf(vec);
    return hnsw_id;
}

std::vector<VerifiedResult> VerifiableVectorDB::query(const std::vector<float>& q, int K) {
    auto candidates = index.search(q, K);

    std::vector<VerifiedResult> results;
    for(const auto& cand : candidates) {
        VerifiedResult res;
        res.id = cand.second;
        res.vector_data = index.get_vector(res.id);
        res.merkle_proof = merkle_tree.get_proof(res.id);

        results.push_back(res);
    }

    return results;
}

std::string VerifiableVectorDB::get_root() {
    return merkle_tree.get_root();
}