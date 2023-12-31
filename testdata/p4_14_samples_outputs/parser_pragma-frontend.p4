@name(".$InstanceType") enum bit<32> InstanceType_0 {
    START = 32w0,
    start_e2e_mirrored = 32w1,
    start_i2e_mirrored = 32w2
}

#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct metadata {
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start_0 {
        transition accept;
    }
    @packet_entry @name(".start_e2e_mirrored") state start_e2e_mirrored {
        transition accept;
    }
    @packet_entry @name(".start_i2e_mirrored") state start_i2e_mirrored {
        transition accept;
    }
    @name(".$start") state start {
        transition select((InstanceType_0)standard_metadata.instance_type) {
            InstanceType_0.START: start_0;
            InstanceType_0.start_e2e_mirrored: start_e2e_mirrored;
            InstanceType_0.start_i2e_mirrored: start_i2e_mirrored;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".nop") action nop() {
    }
    @name(".exact") table exact_1 {
        actions = {
            nop();
            @defaultonly NoAction_1();
        }
        key = {
            standard_metadata.egress_spec: exact @name("standard_metadata.egress_spec");
        }
        default_action = NoAction_1();
    }
    apply {
        exact_1.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
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
