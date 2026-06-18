#pragma once

#include <vector>
#include <string>

class MerkleTree {
public:
    MerkleTree();

    int append_leaf(const std::vector<float>& vec_data);

    std::string get_root();

    std::vector<std::string> get_proof(int leaf_index);

private:
    std::vector<std::string> leaves;
    std::vector<std::vector<std::string>> tree;
    bool is_dirty;

    std::string hash_vector(const std::vector<float>& vec_data) const;
    std::string hash_pair(const std::string& left, const std::string& right) const;
    void rebuild_tree();
};
