#ifndef _SIMPLE_HASH_CACHE_
#define _SIMPLE_HASH_CACHE_

#include <string.h>
#include <mutex>
#include <assert.h>
#include <functional>
#include <vector>

//PHP采用的优化版本的time33 hash函数. c++标准库的hash不如这个好用;另有 BKDRHash 算法(相比time33种子是33，而BKDRHash的种子是131). 
inline unsigned int time33(char const* str, int len) 
{ 
    unsigned long hash = 5381; 
    /* variant with the hash unrolled eight times */ 
    for (; len >= 8; len -= 8) { 
        hash = ((hash << 5) + hash) + *str++; 
        hash = ((hash << 5) + hash) + *str++; 
        hash = ((hash << 5) + hash) + *str++; 
        hash = ((hash << 5) + hash) + *str++; 
        hash = ((hash << 5) + hash) + *str++; 
        hash = ((hash << 5) + hash) + *str++; 
        hash = ((hash << 5) + hash) + *str++; 
        hash = ((hash << 5) + hash) + *str++; 
    }
    switch (len) { 
        case 7: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */ 
        case 6: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */ 
        case 5: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */ 
        case 4: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */ 
        case 3: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */ 
        case 2: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */ 
        case 1: hash = ((hash << 5) + hash) + *str++; break; 
        case 0: break; 
    } 
    return hash; 
} 

//这个函数的作用是将一个大整数打散，而后映射到一个小的数组范围内. 打散算法的数学原理我未深入研究 
//@x  值; @array_len, 数组大小, 必须是2的倍数
inline unsigned int hash_mod( unsigned int x, unsigned int array_len = 128 ) {
    //打散算法1：  http://java-performance.info/implementing-world-fastest-java-int-to-int-hash-map/
    //int h = x * 0x9E3779B9;
    //h = h ^ (h >> 16);

    //打散算法2:
    //x = ((x >> 16) ^ x) * 0x45d9f3b;
    //x = ((x >> 16) ^ x) * 0x45d9f3b;
    //x = ((x >> 16) ^ x);
    //int h = x;

    //打散算法3，最优
    int h = x*2654435761;

    //映射，即优化版的求余(%), 要求 array_len 是2的倍数
    return h & (array_len - 1);
    //return h % array_len;
}

template<typename T, int array_size=128>
class Simple_Cache {
private:
    typedef T Node_Value_Ptr;

    struct Node {
        char key[56];           //key最大为：56个字符
        int  key_len;
        Node_Value_Ptr value;   //缓存, 现在必须是一个指针
    };

    volatile Node data_[array_size];// = {};  //不能正常初始化? 改到构造函数去
    volatile int  cache_count_;
    std::mutex mutex_;
    std::function<Node_Value_Ptr (const std::string& key)> create_call_back_;

public:
    Simple_Cache() {
        clear();
        set_create_callback(&Simple_Cache::_fake_creator_, this);
    }

    void clear() {
        for (int i=0; i<array_size; ++i) {
            data_[i].key[0]  = '\0';
            data_[i].key_len = 0;
            data_[i].value   = nullptr;
        }
        cache_count_ = 0;
    }

    //get cache object by @key. if no cache object was found then call create_call_back_ function to create one
    Node_Value_Ptr get(const std::string& key) {
        unsigned int h     = time33(key.c_str(), key.length());
        unsigned int index = hash_mod(h, array_size);

        auto r = read(key, index);
        if (r == nullptr) {
            std::unique_lock<std::mutex> lock;
            r = read(key, index); //double check
            if (r == nullptr) {
                return write(key, index);
            }
        }

        return r;
    }

    //test if @key is in cache
    inline bool has_key(const std::string& key) {
        unsigned int h     = time33(key.c_str(), key.length());
        unsigned int index = hash_mod(h, array_size);
        auto r = this->read(key, index);
        return (r != nullptr);
    }

    //cached object size
    int cached_size() const {
        return cache_count_;
    }

    //get all the cached objects
    std::vector<Node_Value_Ptr> cached_object() const {
        std::vector<Node_Value_Ptr> v;
        for (int i=0; i<array_size; ++i) {
            if (data_[i].value != nullptr) {
                v.push_back(data_[i].value);
            }
        }
        return v;
    }

    //max size
    int max_size() const {
        return array_size;
    }

    //在回调函数中创建需要缓存的对象. 绑定类的成员函数, 函数原型: Node_Value Foo::foo(const std::string& key);
    template<typename F1, typename F2>
    void set_create_callback(F1 && f1, F2 && f2) {
        create_call_back_ = std::bind(std::forward<F1>(f1), std::forward<F2>(f2), std::placeholders::_1);
    }

    //在回调函数中创建需要缓存的对象. 绑定一个独立的函数，函数原型: Node_Value foo(const std::string& key);
    template<typename F1>
    void set_create_callback(F1 && f1) {
        create_call_back_ = std::bind(std::forward<F1>(f1), std::placeholders::_1);
    }

private:
    int fake_ = 0;
    Node_Value_Ptr _fake_creator_(const std::string& key) {
        //static int fake = 0;
        //return (int*)time33(key.c_str(), key.length());
        return (Node_Value_Ptr)(++fake_); //cast int to pointer
    }

    //
    Node_Value_Ptr read(const std::string& key, unsigned int index) {
        int n = index;
        //if (data_[n].value == nullptr) {
        //    return nullptr;
        //}

        //判断是否有数据，并处理hash冲突
        while ( data_[n].key_len > 0 && (data_[n].key_len != key.length() || 0 != strncmp((char*)data_[n].key, key.c_str(), data_[n].key_len)) ) {
            n = (n + 1) & (array_size - 1);
            if (n == index) {
                //assert(false)
                return nullptr; //警告：：个数超出 array_size
            }
        }
        return data_[n].value;
    }

    Node_Value_Ptr write(const std::string& key, unsigned int index) {
        if (cache_count_ >= array_size) {
            assert(false && "write error1::cache object more than array_size!");
        }

        int n = index;

        //先处理hash冲突
        while (data_[n].value != nullptr) {
            n = (n + 1) & (array_size - 1);  //means: (n+1) % array_size
            if (n == index) {
                assert(false && "write error2::cache object more than array_size!");  //超出个数啦
            }
        }

        Node_Value_Ptr v = create_call_back_(key);  //生成缓存对象, 现在必须是指针
        if (nullptr == v) {  //不能生成??失败?? 
            return nullptr;
        }

        data_[n].value   = v;
        data_[n].key_len = key.length();
        strncpy((char*)data_[n].key, key.c_str(), sizeof(data_[n].key) - 1);

        ++cache_count_;
        return v;
    }

};

#endif 

