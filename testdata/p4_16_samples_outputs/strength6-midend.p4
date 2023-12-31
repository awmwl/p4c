#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<16> a;
    bit<16> b;
    bit<8>  c;
}

struct Headers {
    hdr h;
}

struct Meta {
    bit<8> x;
    bit<8> y;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.case0") action case0() {
        h.h.c = h.h.a[11:4];
    }
    @name("ingress.case1") action case1() {
        h.h.c = (bit<8>)h.h.a[11:7];
    }
    @name("ingress.case2") action case2() {
        h.h.c = 8w0;
    }
    @name("ingress.case3") action case3() {
        h.h.c = h.h.a[12:5];
    }
    @name("ingress.case4") action case4() {
        h.h.c = ((int<32>)(int<16>)h.h.a)[22:15];
    }
    @name("ingress.t") table t_0 {
        actions = {
            case0();
            case1();
            case2();
            case3();
            case4();
        }
        const default_action = case0();
    }
    apply {
        t_0.apply();
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract<hdr>(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<hdr>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
