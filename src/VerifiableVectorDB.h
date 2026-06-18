#pragma once

#include "HNSW.h"
#include "MerkleTree.h"
#include <vector>
#include <string>

struct VerifiedResult {
    int id;
    std::vector<float> vector_data;
    std::vector<std::string> merkle_proof;
};

class VerifiableVectorDB {
public:
    VerifiableVectorDB(int M = 16, int ef_construct = 50);

    int insert(const std::vector<float>& vec);

    std::vector<VerifiedResult> query(const std::vector<float>& q, int K);

private:
    SimpleHNSW index;
    MerkleTree merkle_tree;
};
