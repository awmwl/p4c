#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr1;
    ethernet_t eth_hdr2;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract<ethernet_t>(hdr.eth_hdr1);
        pkt.extract<ethernet_t>(hdr.eth_hdr2);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    ethernet_t tmp1;
    ethernet_t tmp2;
    apply {
        tmp1 = (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = 16w1};
        h.eth_hdr1 = tmp1;
        tmp2.setValid();
        tmp2.dst_addr = 48w1;
        tmp2.src_addr = 48w1;
        tmp2.eth_type = 16w1;
        h.eth_hdr2 = tmp2;
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

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
