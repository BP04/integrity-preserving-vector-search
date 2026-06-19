#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

inline std::string client_sha256(const std::string& data) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    EVP_DigestInit_ex(context, EVP_sha256(), nullptr);
    EVP_DigestUpdate(context, data.data(), data.size());
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;
    EVP_DigestFinal_ex(context, hash, &lengthOfHash);
    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for(unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

inline bool verify_merkle_proof(const std::vector<float>& leaf_data, int leaf_idx,
                                const std::vector<std::string>& proof, const std::string& trusted_root) {
    std::string raw_bytes(reinterpret_cast<const char*>(leaf_data.data()), leaf_data.size() * sizeof(float));
    std::string current_hash = client_sha256(raw_bytes);

    int current_idx = leaf_idx;

    for(const std::string& sibling_hash : proof) {
        bool is_right_child = (current_idx % 2 != 0);
        if(is_right_child) {
            current_hash = client_sha256(sibling_hash + current_hash);
        }
        else {
            current_hash = client_sha256(current_hash + sibling_hash);
        }
        current_idx /= 2;
    }

    return current_hash == trusted_root;
}
