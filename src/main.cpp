
//////////////////////////////////////////////////////////////////////////


#include <iostream>
#include <stdio.h>
#include <map>
#include <algorithm>
#include "simple_cache.h"

using namespace std;



int g_xx_go = 0;
int* foo(const std::string& key) {
    //return (int*)time33(key.c_str(), key.length());
    return (int*)(++g_xx_go);
}

class Foo {
public:
    int* make_foo(const std::string& key) {
        return new int;
    }
};


int main(int argc, char *argv[])
{
	//char sz[] = "Hello, World!";	//Hover mouse over "sz" while debugging to see its contents
	//cout << sz << endl;	//<================= Put a breakpoint here

    //int a = std::hash();
    std::hash<char*> hash_fn;

    char* test_data[] = {
        
        "good1", 
        "nice", 
        "great", 
        "good", 
        "Surprisingly", 
        "many",
        "people",
        "think", 
        "that", 
        "old", 
        "wisdom", 
        "about", 
        "extremely"
    };
    
    std::map<int, char*> map;
    std::vector<int> vec;
    int error = 0;

    int n = sizeof(test_data) / sizeof(test_data[0]);
    for (int i=0; i<n; ++i) {
        char *data = test_data[i];
        unsigned int len = strlen(data);
        //unsigned int h = std::hash<char*>()(data);
        unsigned int h = time33(data, len);
        //unsigned int h = BKDRHash(data);

        int xx2 = hash_mod(h);
        cout << data << "\t\t" << h << "\t\t" << xx2 << "\t\t"/* << small2(h, 128) */<< endl;

        vec.push_back(xx2);
        if (map.find(xx2) != map.end()) {
            cout << "key collision-->" << h << "\t" << xx2 << "\t" << map[xx2] << endl;
            error++;
        }
        map[xx2] = data;
    }
    cout << "error = " << error << endl;

    std::sort(vec.begin(), vec.end());
    for (auto i=vec.begin(); i!=vec.end(); ++i) {
        cout << *i << ",";
    }
    cout << endl;

    //test case 1
    Simple_Cache<int*> aaa;
    //aaa.set_create_callback(&foo);

    std::vector<int*> vv;
    for (int i=0; i<n; ++i) {
        char *data = test_data[i];
        int* ptr = aaa.get(std::string(data));
        vv.push_back(ptr);
    }

    std::sort(vv.begin(), vv.end());
    for (auto i=vv.begin(); i!=vv.end(); ++i) {
        cout << (size_t)(*i) << ",";
    }
    cout << endl;

    assert(aaa.has_key("good"));
    assert(aaa.has_key("wisdom"));
    assert(!aaa.has_key("not_have_yet"));

    //test case 2
    Simple_Cache<int*> bbb;
    int *p1 = bbb.get("a1");
    int *p2 = bbb.get("a2");
    int *p3 = bbb.get("a3");
    assert((size_t)p1 == 1);
    assert((size_t)p2 == 2);
    assert((size_t)p3 == 3);
    int *p1_1 = bbb.get("a1");
    assert((size_t)p1_1 == 1);

    //Foo foo1;
    //aaa.set_create_callback(&Foo::make_foo, &foo1);

    //aaa.bind_create_callback(&foo);
    //auto f = std::bind(&foo2, 1, 2);
    //aaa.bind_create_callback(std::bind(&foo2, 1, 2));

    //auto f1 = std::bind(&Foo::make_foo, &foo, );

    //aaa.bind_create_callback(&foo, "a", 100);


    //Foo foo1;
    ////aaa.bind_create_callback(&Foo::make_foo, &foo, std::placeholders::_1 );
    //aaa.bind_create_callback(&Foo::make_foo, &foo1, "a", 100);

    //CALLBACK_TYPE f2 = std::bind(&Foo::make_foo, &foo1, std::placeholders::_1, 100);
    //aaa.bind_create_callback_2(std::move(f2));
    //aaa.set_create_callback(&Foo::make_foo, &foo1);
    
    //int a = hash_fn("goood");

    //int* x1 = aaa.get("5");

	return 0;
}

