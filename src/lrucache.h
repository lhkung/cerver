#ifndef LRU_CACHE_H_
#define LRU_CACHE_H_

#include <list>
#include <unordered_map>
#include <memory>
#include <string>
#include <pthread.h>

template <typename K, typename V>
class LRUCache {
  public:
    LRUCache(size_t capacity) : capacity_(capacity), size_(0) {
      pthread_mutex_init(&lock_, nullptr);
    }
    virtual ~LRUCache() {
      pthread_mutex_destroy(&lock_);
    }

    int Put(const K& key, const V& val) {
      if (sizeof(val) > capacity_) {
        return -1;
      }
      pthread_mutex_lock(&lock_);
      auto it = map_.find(key);
      if (it == map_.end()) { // New element
        while (size_ + sizeof(val) >= capacity_) {
          Evict();
        }
        queue_.push_back(key);
        auto last = queue_.end();
        last--;
        map_.insert({key, last});
        kv_.insert({key, val});
        size_ += sizeof(val);
        pthread_mutex_unlock(&lock_);
        return 0;
      } else { // Existing element
        size_ -= sizeof(kv_.at(key));
        kv_[key] = val;
        size_ += sizeof(val);
        queue_.splice(queue_.end(), queue_, it->second);
        pthread_mutex_unlock(&lock_);
        return 1;
      }
    }
    int Get(const K& key, V* val) {
      pthread_mutex_lock(&lock_);
      auto it = map_.find(key);
      if (it == map_.end()) { // Does not exist
        pthread_mutex_unlock(&lock_);
        return -1;
      }
      *val = kv_.at(it->first);
      // LRU cache tracks Get access.
      queue_.splice(queue_.end(), queue_, it->second);
      pthread_mutex_unlock(&lock_);
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
      size_ -= sizeof(kv_.at(*first_it));
      kv_.erase(*first_it);
      map_.erase(*first_it);
      queue_.pop_front();
    }
    size_t capacity_;
    size_t size_;
    typename std::list<K> queue_;
    std::unordered_map<K, typename std::list<K>::iterator> map_;
    typename std::unordered_map<K, V> kv_;
    pthread_mutex_t lock_;
};

class LRUStringCache {
  public:
    LRUStringCache(size_t capacity) : capacity_(capacity), size_(0) {
      pthread_mutex_init(&lock_, nullptr);
    }
    virtual ~LRUStringCache() {
      pthread_mutex_destroy(&lock_);
    }
    int Put(const std::string& key, const std::string& val) {
      if (val.size() > capacity_) {
        return -1;
      }
      pthread_mutex_lock(&lock_);
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
        pthread_mutex_unlock(&lock_);
        return 0;
      } else { // Existing element
        size_ -= kv_.at(key).length();
        kv_[key] = val;
        size_ += val.length();
        queue_.splice(queue_.end(), queue_, it->second);
        pthread_mutex_unlock(&lock_);
        return 1;
      }
    }

    int Get(const std::string& key, std::string* val) {
      pthread_mutex_lock(&lock_);
      auto it = map_.find(key);
      if (it == map_.end()) { // Does not exist
        pthread_mutex_unlock(&lock_);
        return -1;
      }
      *val = kv_.at(it->first);
      // LRU cache tracks Get access.
      queue_.splice(queue_.end(), queue_, it->second);
      pthread_mutex_unlock(&lock_);
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

    void GetKeys(std::string* keys) {
      for (auto it = queue_.begin(); it != queue_.end(); it++) {
        *keys += *it + "<br>";
      } 
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
    pthread_mutex_t lock_;
};

#endif