#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

header IDX {
    bit<8> idx;
}

struct Headers {
    ethernet_t eth_hdr;
    IDX        idx;
    H[2]       h;
}

bit<8> bound(in bit<8> val, in bit<8> bound) {
    return (val < bound ? val : bound);
}
struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.idx);
        pkt.extract(hdr.h.next);
        pkt.extract(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action simple_action(bool check) {
        if (check) {
            bit<8> val = bound(h.idx.idx, 8w1);
            h.h[val].a = 8w1;
        }
    }
    apply {
        simple_action(true);
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
        b.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
