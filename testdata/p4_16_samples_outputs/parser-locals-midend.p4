#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

struct S {
    H       h1;
    H       h2;
    bit<32> c;
}

parser p() {
    H s_0_h1;
    H s_0_h2;
    state start {
        s_0_h1.setInvalid();
        s_0_h2.setInvalid();
        transition accept;
    }
}

parser empty();
package top(empty e);
top(p()) main;
