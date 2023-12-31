#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> mac_addr_t;
header aggregator_t {
    bit<8> base0;
    bit<8> base1;
    bit<8> base2;
    bit<8> base3;
    bit<8> val;
}

header vec_e_t {
    bit<8> e;
}

header ml_hdr_t {
    int<8> idx;
}

header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16>    etherType;
}

struct headers {
    ethernet_t      ethernet;
    ml_hdr_t        ml;
    vec_e_t[3]      vector;
    aggregator_t[3] pool;
}

struct metadata_t {
    int<8> counter;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        packet.extract(hdr.ml);
        packet.extract(hdr.vector[0]);
        packet.extract(hdr.vector[1]);
        packet.extract(hdr.vector[2]);
        packet.extract(hdr.pool[0]);
        packet.extract(hdr.pool[1]);
        packet.extract(hdr.pool[2]);
        meta.counter = 0;
        transition accept;
    }
}

enum int<8> Index_t {
    ZERO = 0,
    ONE = 1,
    TWO = 2
}

control ingress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        meta.counter = meta.counter + 1;
        hdr.vector[0].e = hdr.pool[1].val + 1;
        Index_t i = (Index_t)hdr.ml.idx;
        hdr.pool[i].val = hdr.vector[0].e;
        hdr.pool[i].base2 = hdr.vector[0].e;
        hdr.vector[1].e = hdr.pool[i].base0;
        hdr.pool[i].base0 = hdr.pool[i].base1 + 1;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
}

control egress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), ingress(), egress(), MyComputeChecksum(), MyDeparser()) main;
