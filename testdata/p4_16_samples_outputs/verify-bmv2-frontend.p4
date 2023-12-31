error {
    NewError
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct m {
    int<8> x;
}

struct h {
}

parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t std) {
    @name("MyParser.e") error e_0;
    state start {
        verify(meta.x == 8s0, error.NewError);
        verify(true, error.NoError);
        e_0 = error.NoError;
        verify(true, e_0);
        transition accept;
    }
}

control MyVerifyChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyDeparser(packet_out b, in h hdr) {
    apply {
    }
}

V1Switch<h, m>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
