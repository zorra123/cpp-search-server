#pragma once
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <vector>


using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct SubMap {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, SubMap& submap)
            : guard(submap.mutex)
            , ref_to_value(submap.map[key]) {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        : map_vec(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto& sub_map = map_vec[static_cast<uint64_t>(key) % map_vec.size()];
        return { key, sub_map };
    }

    void erase(const Key& key)
    {
        auto& sub_map = map_vec[static_cast<uint64_t>(key) % map_vec.size()];
        std::lock_guard guard(sub_map.mutex);
        sub_map.map.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : map_vec) {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    std::vector<SubMap> map_vec;
};


template <typename Value>
class ConcurrentSet
{
private:
    struct SubSet
    {
        std::mutex mutex_for_set;
        std::set<Value> set_origin;
    };

    std::vector<SubSet> subsets_veс;
    size_t size_ = 0;
public:

    struct Access
    {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const int index, SubSet& subset) :
            guard(subset.mutex_for_set),
            ref_to_value(subset.set_origin[index])
        {}
    };

    explicit ConcurrentSet(size_t subset_count) :subsets_veс(subset_count)
    {}
    
    Access operator[](const int index)
    {
        SubSet& subset = subsets_veс[static_cast<uint64_t>(index) % subsets_veс.size()];

        return { index, subset };
    }

    void insert(const Value& value)
    {
        SubSet& subset = subsets_veс[static_cast<uint64_t>(size_) % subsets_veс.size()];
        std::lock_guard guard(subset.mutex_for_set);
        subset.set_origin.insert(value);
        ++size_;
    }

    std::set<Value> BuildOrdinarySet()
    {
        std::set<Value> result;
        for (auto& [mutex_for_set, set_for_thread] : subsets_veс)
        {
            std::lock_guard g(mutex_for_set);
            result.insert(set_for_thread.begin(), set_for_thread.end());
        }
        return result;
    }
};
