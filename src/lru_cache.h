#ifndef LRU_CACHE_H_
#define LRU_CACHE_H_

#include <list>
#include <unordered_map>
#include <memory>
#include <string>

class LRUCache {
  public:
    LRUCache() : capacity_(1024), size_(0) {}
    LRUCache(size_t capacity) : capacity_(capacity), size_(0) {}
    virtual ~LRUCache() {}
    int Put(const std::string& key, const std::string& val) {
      if (val.size() > capacity_) {
        return -1;
      }
      auto it = map_.find(key);
      if (it == map_.end()) { // New element
        while (size_ + val.size() >= capacity_) {
          Evict();
        }
        queue_.push_back(key);
        auto last = queue_.end();
        last--;
        map_.insert({key, last});
        kv_.insert({key, val});
        size_ += val.length();
        return 0;
      } else { // Existing element
        size_ -= kv_.at(it->first).length();
        kv_[it->first] = val;
        size_ += val.length();
        queue_.splice(queue_.end(), queue_, it->second);
        return 1;
      }
    }
    int Get(const std::string& key, std::string* val) {
      auto it = map_.find(key);
      if (it == map_.end()) { // Does not exist
        return -1;
      }
      *val = kv_.at(it->first);
      // LRU cache tracks Get access.
      queue_.splice(queue_.end(), queue_, it->second);
      return 0;
    }
    size_t Capacity() {
      return capacity_;
    }
    size_t Size() {
      return size_;
    }
    int Entry() {
      return queue_.size();
    }
  private:
    void Evict() {
      auto first_it = queue_.begin();
      size_ -= kv_.at(*first_it).length();
      kv_.erase(*first_it);
      map_.erase(*first_it);
      queue_.pop_front();
    }
    size_t capacity_;
    size_t size_;
    typename std::list<std::string> queue_;
    typename std::unordered_map<std::string, std::string> kv_;
    std::unordered_map<std::string, typename std::list<std::string>::iterator> map_;
};

#endif