#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"

#include <cstddef>
#include <new>

namespace Spark {

template<typename K>
struct DefaultHash {
    [[nodiscard]] std::size_t operator()(const K& key) const noexcept {
        const auto* bytes = reinterpret_cast<const unsigned char*>(&key);
        std::size_t h = 5381;
        for (std::size_t i = 0; i < sizeof(K); ++i) {
            h = ((h << 5) + h) + bytes[i];
        }
        return h;
    }
};

template<typename K>
struct DefaultKeyEqual {
    [[nodiscard]] bool operator()(const K& a, const K& b) const noexcept { return a == b; }
};

template<typename K, typename V>
struct HashMapNode {
    K key;
    V value;
    HashMapNode* next = nullptr;
};

/**
 * Separate chaining hash table. Non-copyable; movable.
 */
template<typename K, typename V, typename Hasher = DefaultHash<K>, typename KeyEq = DefaultKeyEqual<K>>
class HashMap {
public:
    HashMap() = default;

    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    HashMap(HashMap&& other) noexcept
        : buckets(MoveTemp(other.buckets)), numEntries(other.numEntries), hasher(other.hasher), eq(other.eq) {
        other.numEntries = 0;
    }

    HashMap& operator=(HashMap&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Clear();
        buckets = MoveTemp(other.buckets);
        numEntries = other.numEntries;
        hasher = other.hasher;
        eq = other.eq;
        other.numEntries = 0;
        return *this;
    }

    ~HashMap() { Clear(); }

    [[nodiscard]] std::size_t GetSize() const noexcept { return numEntries; }
    [[nodiscard]] bool IsEmpty() const noexcept { return numEntries == 0; }

    void Clear() {
        ClearNodes();
        buckets.Clear();  // drop bucket array so next Add starts fresh
    }

    void Reserve(std::size_t bucketHint) {
        if (bucketHint == 0 || buckets.GetSize() >= bucketHint) {
            return;
        }
        Rehash(bucketHint);
    }

    [[nodiscard]] V* Find(const K& key) {
        if (buckets.IsEmpty()) {
            return nullptr;
        }
        const std::size_t b = BucketIndex(key);
        for (HashMapNode<K, V>* node = buckets[b]; node != nullptr; node = node->next) {
            if (eq(node->key, key)) {
                return &node->value;
            }
        }
        return nullptr;
    }

    [[nodiscard]] const V* Find(const K& key) const {
        return const_cast<HashMap*>(this)->Find(key);
    }

    void Add(const K& key, const V& value) {
        if (V* existing = Find(key)) {
            *existing = value;
            return;
        }
        EnsureLoad();
        const std::size_t b = BucketIndex(key);
        auto* node = static_cast<HashMapNode<K, V>*>(::operator new(sizeof(HashMapNode<K, V>)));
        new (&node->key) K(key);
        new (&node->value) V(value);
        node->next = buckets[b];
        buckets[b] = node;
        ++numEntries;
    }

    void Add(K&& key, V&& value) {
        if (V* existing = Find(key)) {
            *existing = MoveTemp(value);
            return;
        }
        EnsureLoad();
        const std::size_t b = BucketIndex(key);
        auto* node = static_cast<HashMapNode<K, V>*>(::operator new(sizeof(HashMapNode<K, V>)));
        new (&node->key) K(MoveTemp(key));
        new (&node->value) V(MoveTemp(value));
        node->next = buckets[b];
        buckets[b] = node;
        ++numEntries;
    }

    bool Remove(const K& key) {
        if (buckets.IsEmpty()) {
            return false;
        }
        const std::size_t b = BucketIndex(key);
        HashMapNode<K, V>* prev = nullptr;
        HashMapNode<K, V>* node = buckets[b];
        while (node != nullptr) {
            if (eq(node->key, key)) {
                if (prev != nullptr) {
                    prev->next = node->next;
                } else {
                    buckets[b] = node->next;
                }
                node->key.~K();
                node->value.~V();
                ::operator delete(node);
                --numEntries;
                return true;
            }
            prev = node;
            node = node->next;
        }
        return false;
    }

private:
    Array<HashMapNode<K, V>*> buckets;
    std::size_t numEntries = 0;
    Hasher hasher{};
    KeyEq eq{};

    [[nodiscard]] std::size_t BucketIndex(const K& key) const noexcept {
        return hasher(key) % buckets.GetSize();
    }

    void InitBuckets(std::size_t n) {
        buckets.Resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            buckets[i] = nullptr;
        }
    }

    void EnsureLoad() {
        constexpr std::size_t kInitialBuckets = 16;
        if (buckets.IsEmpty()) {
            InitBuckets(kInitialBuckets);
            return;
        }
        const std::size_t b = buckets.GetSize();
        if (numEntries * 4 > b * 3) {
            Rehash(b * 2);
        }
    }

    void Rehash(std::size_t newBucketCount) {
        Array<HashMapNode<K, V>*> old = MoveTemp(buckets);
        InitBuckets(newBucketCount);
        numEntries = 0;
        for (std::size_t i = 0; i < old.GetSize(); ++i) {
            HashMapNode<K, V>* node = old[i];
            while (node != nullptr) {
                HashMapNode<K, V>* next = node->next;
                node->next = nullptr;
                const std::size_t b = BucketIndex(node->key);
                node->next = buckets[b];
                buckets[b] = node;
                node = next;
                ++numEntries;
            }
        }
    }

    void ClearNodes() noexcept {
        for (std::size_t i = 0; i < buckets.GetSize(); ++i) {
            HashMapNode<K, V>* node = buckets[i];
            while (node != nullptr) {
                HashMapNode<K, V>* next = node->next;
                node->key.~K();
                node->value.~V();
                ::operator delete(node);
                node = next;
            }
            buckets[i] = nullptr;
        }
        numEntries = 0;
    }
};

}  // namespace Spark
