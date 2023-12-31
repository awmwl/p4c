/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<32> a;
    bit<32> b;
    bit<64> c;
}

struct Headers {
    hdr h;
}

struct Meta {}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h.h); }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action add()
    { h.h.c = (bit<64>)(h.h.a + h.h.b); sm.egress_spec = 0; }
    action sub()
    { h.h.c = (bit<64>)(h.h.a - h.h.b); sm.egress_spec = 0; }
    table t {
        actions = { add; sub; }
        const default_action = add();
    }
    table t1 {
        actions = { sub; }
        const default_action = sub();
    }
    table t2 {
        actions = { add; }
        const default_action = add();
    }
    apply {
        switch (t.apply().action_run) {
            add: {
            }
            default: {
                t1.apply();
            }
        }
        t2.apply();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
