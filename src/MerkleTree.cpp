#include "MerkleTree.h"
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

static std::string sha256(const std::string& data) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if(context == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if(EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1 ||
       EVP_DigestUpdate(context, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("EVP Digest update failed");
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if(EVP_DigestFinal_ex(context, hash, &lengthOfHash) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("EVP Digest final failed");
    }
    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for(unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

MerkleTree::MerkleTree() : is_dirty(false) {}

std::string MerkleTree::hash_vector(const std::vector<float>& vec_data) const {
    std::string raw_bytes(reinterpret_cast<const char*>(vec_data.data()), vec_data.size() * sizeof(float));
    return sha256(raw_bytes);
}

std::string MerkleTree::hash_pair(const std::string& left, const std::string& right) const {
    return sha256(left + right);
}

int MerkleTree::append_leaf(const std::vector<float>& vec_data) {
    std::string leaf_hash = hash_vector(vec_data);
    leaves.push_back(leaf_hash);
    is_dirty = true;
    return leaves.size() - 1;
}

void MerkleTree::rebuild_tree() {
    tree.clear();
    if(leaves.empty()) {
        is_dirty = false;
        return;
    }

    tree.push_back(leaves);
    int current_level = 0;

    while(tree[current_level].size() > 1) {
        std::vector<std::string> next_level;
        const auto& current_nodes = tree[current_level];

        for(size_t i = 0; i < current_nodes.size(); i += 2) {
            if(i + 1 < current_nodes.size()) {
                next_level.push_back(hash_pair(current_nodes[i], current_nodes[i + 1]));
            } else {
                next_level.push_back(hash_pair(current_nodes[i], current_nodes[i]));
            }
        }
        tree.push_back(next_level);
        current_level++;
    }
    is_dirty = false;
}

std::string MerkleTree::get_root() {
    if(is_dirty) {
        rebuild_tree();
    }
    if(tree.empty() || tree.back().empty()) {
        return "";
    }
    return tree.back()[0];
}

std::vector<std::string> MerkleTree::get_proof(int leaf_index) {
    if(leaf_index < 0 || leaf_index >= static_cast<int>(leaves.size())) {
        throw std::out_of_range("Leaf index out of bounds");
    }

    if(is_dirty) {
        rebuild_tree();
    }

    std::vector<std::string> proof;
    int current_idx = leaf_index;

    for(size_t level = 0; level < tree.size() - 1; ++level) {
        bool is_right_child = (current_idx % 2 != 0);
        int sibling_idx = is_right_child ? (current_idx - 1) : (current_idx + 1);

        if(sibling_idx >= static_cast<int>(tree[level].size())) {
            proof.push_back(tree[level][current_idx]);
        } else {
            proof.push_back(tree[level][sibling_idx]);
        }

        current_idx /= 2;
    }

    return proof;
}
