#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header h_t {
    bit<8> f;
}

struct metadata {
}

struct headers {
    @name(".h")
    h_t h;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<h_t>(hdr.h);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".exact") table exact_0 {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        key = {
            hdr.h.f: ternary @name("h.f");
        }
        size = 256;
        default_action = NoAction();
    }
    apply {
        exact_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<h_t>(hdr.h);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
