struct S<T> {
    T field;
}

extern S<V> f<V>();
control c(out bit<32> o) {
    apply {
        S<bit<32>> s;
        s = f<bit<32>>();
        o = s.field;
    }
}

control _c(out bit<32> o);
package top(_c c);
top(c()) main;
