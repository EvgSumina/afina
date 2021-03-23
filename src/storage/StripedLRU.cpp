#include <stdexcept>
#include "StripedLRU.h"

namespace Afina {
namespace Backend {
    
    std::unique_ptr<StripedLRU> 
    StripedLRU::CreateStripedLRU(std::size_t memory_limit, std::size_t sharde_count) {
        std::size_t sharde_size = memory_limit / sharde_count;
        if (sharde_size < MIN_SHARDE_SIZE) {
            throw std::runtime_error("Little sharde size");
        }
        return std::unique_ptr<StripedLRU>(new StripedLRU(sharde_size, sharde_count));
    };

    // Implements Afina::Storage interface
    bool StripedLRU::Put(const std::string &key, const std::string &value) {
        return _shardes[_hash_shardes(key) % _shardes_cnt]->Put(key, value);
    }

    // Implements Afina::Storage interface
    bool StripedLRU::PutIfAbsent(const std::string &key, const std::string &value) {
        return _shardes[_hash_shardes(key) % _shardes_cnt]->PutIfAbsent(key, value);
    }

    // Implements Afina::Storage interface
    bool StripedLRU::Set(const std::string &key, const std::string &value) {
        return _shardes[_hash_shardes(key) % _shardes_cnt]->Set(key, value);
    }

    // Implements Afina::Storage interface
    bool StripedLRU::Delete(const std::string &key) {
        return _shardes[_hash_shardes(key) % _shardes_cnt]->Delete(key);
    }

    // Implements Afina::Storage interface
    bool StripedLRU::Get(const std::string &key, std::string &value) {
        return _shardes[_hash_shardes(key) % _shardes_cnt]->Get(key, value);
    }

} // namespace Backend
} // namespace Afina
