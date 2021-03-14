#ifndef AFINA_STORAGE_STRIPED_SIMPLE_LRU_H
#define AFINA_STORAGE_STRIPED_SIMPLE_LRU_H

#include <string>
#include <functional>
#include <memory>
#include <vector>

#include <afina/Storage.h>
#include "ThreadSafeSimpleLRU.h"

namespace Afina {
namespace Backend {

constexpr std::size_t MIN_SHARDE_SIZE = 1024 * 1024UL;

class StripedLRU : public Afina::Storage {
private:
    std::vector<std::unique_ptr<ThreadSafeSimplLRU>> _shardes;
    std::hash<std::string> _hash_shardes;
    std::size_t _shardes_cnt;

    StripedLRU(std::size_t sharde_size, std::size_t n_shardes): _shardes_cnt{n_shardes} {
        _shardes.reserve(n_shardes);
        for (std::size_t i = 0; i < n_shardes; ++i) {
            _shardes.emplace_back(new ThreadSafeSimplLRU(sharde_size));
        }
    }

public:
    static std::unique_ptr<StripedLRU> 
    CreateStripedLRU(std::size_t memory_limit = 16 * 1024 * 1024UL, 
                    std::size_t sharde_count = 4);

    ~StripedLRU() {}

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_STRIPED_LRU_H
