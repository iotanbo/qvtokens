

#include "../src/app/app.hpp"
#include <assert.h>
#include <memory>
#include <forward_list>
#include <stdio.h>

class Dummy {
public:
    char text[64];
    int num = 0;
    static int id;
    Dummy() {
        num = id++;
        sprintf(text, "text %d\n", num);
        // fprintf(stderr, "Dummy created\n");
    }

    ~Dummy() {
        // fprintf(stderr, "Dummy destructed\n");
    }

    void info() {
        // fprintf(stderr, "Dummy object # %d, [%s]\n", num, text);
    }

};
int Dummy::id = 1;


int _test_forward_list_can_contain_unique_ptr() {
    int result = 0;

    std::forward_list<std::unique_ptr<Dummy>> l;

    for (size_t i=0; i<5; ++i) {
        l.emplace_front(std::make_unique<Dummy>());
    }

    assert(!l.empty());

    std::unique_ptr<Dummy> p1 = std::move(l.front());
    l.pop_front(); 
    p1->info();
    return result; 
}


int test_container_of_unique_ptr() {
    int result = 0;

    result |= _test_forward_list_can_contain_unique_ptr();

    return result;
}
