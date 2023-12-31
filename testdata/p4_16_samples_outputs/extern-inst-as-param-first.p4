#include <core.p4>

extern MyCounter<I> {
    MyCounter(bit<32> size);
    void count(in I index);
}

typedef bit<10> my_counter_index_t;
typedef MyCounter<my_counter_index_t> my_counter_t;
control Inner(my_counter_t counter_set) {
    action count(my_counter_index_t index) {
        counter_set.count(index);
    }
    table counter_table {
        actions = {
            count();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        counter_table.apply();
    }
}

control Test() {
    my_counter_t(32w1024) counter_set;
    @name("Inner") Inner() Inner_inst;
    apply {
        Inner_inst.apply(counter_set);
    }
}

control C();
package P(C _c);
P(Test()) main;
