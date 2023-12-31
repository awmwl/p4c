#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct S {
    bit<8> start;
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

parser p(packet_in pkt, out Headers hdr, inout S s, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        s.start = 8w0;
        transition parse_a;
    }
    @name("p.a") state parse_a {
        s.a = 8w4;
        transition parse_b;
    }
    @name("p.b") state parse_b {
        s.b = 8w4;
        s.c = 8w4;
        transition accept;
    }
}

control ingress(inout Headers h, inout S s, inout standard_metadata_t sm) {
    apply {
    }
}

control vrfy(inout Headers h, inout S s) {
    apply {
    }
}

control update(inout Headers h, inout S s) {
    apply {
    }
}

control egress(inout Headers h, inout S s, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, S>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
