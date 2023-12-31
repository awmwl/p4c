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

@pragma packet_entry
parser start_i2e_mirrored {
    return start;
}

parser start {
    return ingress;
}

header_type start {
    fields {
        f1 : 32;
    }
}

metadata start m;

action a1() {
    modify_field(m.f1, 1);
}

table t1 {
    actions {
        a1;
    }
}

control ingress {
    apply(t1);
}

control egress {
}