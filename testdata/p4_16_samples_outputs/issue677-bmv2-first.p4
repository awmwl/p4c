#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef standard_metadata_t SM;
struct H {
}

struct M {
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout SM smeta) {
    state start {
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout SM smeta) {
    apply {
        smeta.egress_spec = 9w1;
    }
}

control EgressI(inout H hdr, inout M meta, inout SM smeta) {
    apply {
    }
}

control DeparserI(packet_out pk, in H hdr) {
    apply {
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
