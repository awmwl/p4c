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

#ifndef _LIB_ORDERED_SET_H_
#define _LIB_ORDERED_SET_H_

#include <cassert>
#include <functional>
#include <initializer_list>
#include <utility>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/list.hpp>

// Remembers items in insertion order
template <class T, class COMP = std::less<T>, class ALLOC = std::allocator<T>>
class ordered_set {
    using list_base_hook = boost::intrusive::list_base_hook<>;
    using set_base_hook = boost::intrusive::set_base_hook<>;

    struct Node : public list_base_hook, public set_base_hook
    {
        T data;

        template <typename ...U>
        explicit Node(U&&... u) : data(std::forward<U>(u)...) {};

        bool operator<(const Node& r) const {
            return COMP()(data, r.data);
        }
    };

 public:
    typedef T                                                   key_type;
    typedef T                                                   value_type;
    typedef COMP                                                key_compare;
    typedef COMP                                                value_compare;
    typedef typename ALLOC::template rebind<Node>::other        allocator_type;
    typedef const T                                             &reference;
    typedef const T                                             &const_reference;

 private:
    typedef boost::intrusive::list<Node> list_type;
    typedef boost::intrusive::set<Node> set_type;
    typedef std::allocator_traits<allocator_type> alloc_traits;

 public:
    struct iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
        using value_type = T;
        using reference = T&;
        using pointer = T*;

        iterator() = default;
        iterator(typename list_type::iterator it) : base(it) {}

        reference operator*() const { return base->data; }
        pointer operator->() const { return &base->data; }
        iterator operator++() { ++base; return *this; }
        iterator operator++(int) { auto tmp = *this; ++base; return tmp; }
        iterator operator--() { --base; return *this; }
        iterator operator--(int) { auto tmp = *this; --base; return tmp; }
        bool operator==(const iterator& o) const { return base == o.base; }
        bool operator!=(const iterator& o) const { return base != o.base; }

        typename list_type::iterator base;
    };

    struct const_iterator : public std::iterator<std::bidirectional_iterator_tag, const T> {
        using value_type = const T;
        using reference = const T&;
        using pointer = const T*;

        const_iterator() = default;
        const_iterator(iterator it) : base(it.base) {}
        const_iterator(typename list_type::const_iterator it) : base(it) {}

        reference operator*() const { return base->data; }
        pointer operator->() const { return &base->data; }
        const_iterator operator++() { ++base; return *this; }
        const_iterator operator++(int) { auto tmp = *this; ++base; return tmp; }
        const_iterator operator--() { --base; return *this; }
        const_iterator operator--(int) { auto tmp = *this; --base; return tmp; }
        bool operator==(const const_iterator& o) const { return base == o.base; }
        bool operator!=(const const_iterator& o) const { return base != o.base; }

        typename list_type::const_iterator base;
    };

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
    struct KeyValueCompare {
        bool operator()(const Node& l, const T& r) const {
            return COMP()(l.data, r);
        }
        bool operator()(const T& l, const Node& r) const {
            return COMP()(l, r.data);
        }
    };
    struct Disposer {
        Disposer(allocator_type& a) : alloc(a) {}

        void operator()(Node* i) {
             alloc_traits::destroy(alloc, i);
             alloc_traits::deallocate(alloc, i, 1);
        }
        allocator_type alloc;
    };

    allocator_type   alloc;
    Disposer         disposer;
    set_type         data_set;
    list_type        data;

    reverse_iterator make_rev_iter(iterator it) const {
        return reverse_iterator(it);
    }
    const_reverse_iterator make_rev_iter(const_iterator it) const {
        return const_reverse_iterator(it);
    }
    iterator tr_iter(typename set_type::iterator i) {
        if (i == data_set.end())
            return data.end();
        return data.iterator_to(*i);
    }
    const_iterator tr_iter(typename set_type::const_iterator i) const {
        if (i == data_set.end())
            return data.end();
        return data.iterator_to(*i);
    }

    template <typename ...Args>
    Node* construct(Args&&... args) {
        Node* item = alloc_traits::allocate(alloc, 1);
        try {
            alloc_traits::construct(alloc, item, std::forward<Args>(args)...);
        } catch (...) {
            alloc_traits::deallocate(alloc, item, 1);
            throw;
        }
        return item;
    }

 public:
    typedef typename set_type::size_type        size_type;
    class sorted_iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
        friend class ordered_set;
        typename set_type::const_iterator       iter;
        sorted_iterator(typename set_type::const_iterator it)    // NOLINT(runtime/explicit)
        : iter(it) {}
     public:
        const T &operator*() const { return iter->data; }
        const T *operator->() const { return &iter->data; }
        sorted_iterator operator++() { ++iter; return *this; }
        sorted_iterator operator--() { --iter; return *this; }
        sorted_iterator operator++(int) { auto copy = *this; ++iter; return copy; }
        sorted_iterator operator--(int) { auto copy = *this; --iter; return copy; }
        bool operator==(const sorted_iterator i) const { return iter == i.iter; }
        bool operator!=(const sorted_iterator i) const { return iter != i.iter; }
    };

    ordered_set() : disposer(alloc) {}
    ordered_set(const ordered_set &a) : disposer(alloc) {
        for (const auto& e : a.data) {
            Node* new_item = construct(e);
            data_set.insert(*new_item);
            data.push_back(*new_item);
        }
    }
    ordered_set(std::initializer_list<T> init) : disposer(alloc) {
        for (const auto& e : init) {
            Node* new_item = construct(e);
            data.push_back(*new_item);
            data_set.insert(*new_item);
        }
    }
    ordered_set(ordered_set &&a) = default;
    ordered_set &operator=(const ordered_set &a) {
        if (this != &a) {
            data_set.clear();
            data.clear_and_dispose(disposer);
            for (const auto& e : a.data) {
                Node* new_item = construct(e);
                data.push_back(*new_item);
                data_set.insert(*new_item);
            }
        }
        return *this;
    }
    ordered_set &operator=(ordered_set &&a) = default;
    bool operator==(const ordered_set &a) const {
        if (data.size() != a.data.size()) return false;
        auto it_l = data.begin();
        auto it_r = a.data.begin();
        for (; it_l != data.end(); ++it_l, ++it_r) {
            if (it_l->data != it_r->data) return false;
        }
        return true;
    }
    ~ordered_set() {
        clear();
    }
    bool operator!=(const ordered_set &a) const { return !(*this == a); }
    bool operator<(const ordered_set &a) const {
        // we define this to work INDEPENDENT of the order -- so it is possible to have
        // two ordered_sets where !(a < b) && !(b < a) && !(a == b) -- such sets have the
        // same elements but in a different order.  This is generally what you want if you
        // have a set of ordered_sets (or use ordered_set as a map key).
        auto it = a.data_set.begin();
        for (const auto &el : data_set) {
            if (it == a.data_set.end()) return true;
            if (COMP()(el.data, it->data)) return true;
            if (COMP()(it->data, el.data)) return false;
            ++it; }
        return false; }

    // FIXME add allocator and comparator ctors...

    iterator                    begin() noexcept { return data.begin(); }
    const_iterator              begin() const noexcept { return data.begin(); }
    iterator                    end() noexcept { return data.end(); }
    const_iterator              end() const noexcept { return data.end(); }
    reverse_iterator            rbegin() noexcept { return make_rev_iter(end()); }
    const_reverse_iterator      rbegin() const noexcept { return make_rev_iter(end()); }
    reverse_iterator            rend() noexcept { return make_rev_iter(begin()); }
    const_reverse_iterator      rend() const noexcept { return make_rev_iter(begin()); }
    const_iterator              cbegin() const noexcept { return data.cbegin(); }
    const_iterator              cend() const noexcept { return data.cend(); }
    const_reverse_iterator      crbegin() const noexcept { return make_rev_iter(cend()); }
    const_reverse_iterator      crend() const noexcept { return make_rev_iter(cbegin()); }
    sorted_iterator             sorted_begin() const noexcept { return data_set.begin(); }
    sorted_iterator             sorted_end() const noexcept { return data_set.end(); }

    reference front() const noexcept { return data.begin()->data; }
    reference back() const noexcept { return data.rbegin()->data; }

    bool        empty() const noexcept { return data.empty(); }
    size_type   size() const noexcept { return data_set.size(); }
    size_type   max_size() const noexcept { return data_set.max_size(); }
    void        clear() {
        data_set.clear();
        data.clear_and_dispose(disposer);
    }

    iterator        find(const T &a) { return tr_iter(data_set.find(a, KeyValueCompare())); }
    const_iterator  find(const T &a) const { return tr_iter(data_set.find(a, KeyValueCompare())); }
    size_type       count(const T &a) const { return data_set.count(a, KeyValueCompare()); }
    iterator        upper_bound(const T &a) { return tr_iter(data_set.upper_bound(a, KeyValueCompare())); }
    const_iterator  upper_bound(const T &a) const { return tr_iter(data_set.upper_bound(a, KeyValueCompare())); }
    iterator        lower_bound(const T &a) { return tr_iter(data_set.lower_bound(a, KeyValueCompare())); }
    const_iterator  lower_bound(const T &a) const { return tr_iter(data_set.lower_bound(a, KeyValueCompare())); }

    std::pair<iterator, bool> insert(const T &v) {
        auto it = data_set.find(v, KeyValueCompare());
        if (it == data_set.end()) {
            Node* new_item = construct(v);
            auto data_it = data.insert(data.end(), *new_item);
            data_set.insert(*new_item);
            return std::make_pair(iterator(data_it), true); }
        return std::make_pair(tr_iter(it), false); }
    std::pair<iterator, bool> insert(T &&v) {
        auto it = data_set.find(v, KeyValueCompare());
        if (it == data_set.end()) {
            Node* new_item = construct(std::move(v));
            auto data_it = data.insert(data.end(), *new_item);
            data_set.insert(*new_item);
            return std::make_pair(iterator(data_it), true); }
        return std::make_pair(tr_iter(it), false); }
    void insert(ordered_set::const_iterator begin, ordered_set::const_iterator end) {
        for (auto it = begin; it != end; ++it) insert(*it);
    }
    template <typename OtherIter>
    void insert(OtherIter begin, OtherIter end) {
        for (auto it = begin; it != end; ++it)
            insert(*it);
    }
    iterator insert(const_iterator pos, const T &v) {
        auto it = data_set.find(v, KeyValueCompare());
        if (it == data_set.end()) {
            Node* new_item = construct(v);
            auto data_it = data.insert(pos.base, *new_item);
            data_set.insert(*new_item);
            return data_it; }
        return tr_iter(it);
    }
    iterator insert(const_iterator pos, T &&v) {
        auto it = find(v, KeyValueCompare());
        if (it == end()) {
            Node* new_item = construct(std::move(v));
            auto data_it = data.insert(pos.base, *new_item);
            data_set.insert(*new_item);
            return data_it; }
        return tr_iter(it);
    }

    void push_back(const T &v) {
        auto it = data_set.find(v, KeyValueCompare());
        if (it == data_set.end()) {
            insert(data.cend(), v);
        } else {
            data.splice(data.end(), data, data.iterator_to(*it));
        }
    }

    void push_back(T &&v) {
        auto it = data_set.find(v, KeyValueCompare());
        if (it == data_set.end()) {
            insert(data.cend(), v);
        } else {
            data.splice(data.end(), data, data.iterator_to(*it));
        }
    }

    template <class ...Args>
    std::pair<iterator, bool> emplace(Args&& ...args) {
        Node* new_item = construct(std::forward<Args>(args)...);
        auto r = data_set.insert(*new_item);
        if (r.second) {
            auto data_it = data.insert(data.end(), *new_item);
            return std::make_pair(data_it, true);
        } else {
            disposer(new_item);
            auto it = data.iterator_to(*r.first);
            data.splice(data.end(), data, it);
            return std::make_pair(std::prev(data.end()), false);
        }
    }


    template <class ...Args>
    std::pair<iterator, bool> emplace_back(Args&& ...args) {
        return emplace(std::forward<Args>(args)...);
    }

    iterator erase(const_iterator pos) {
        auto list_it = pos.base;
        data_set.erase(*list_it);
        auto ret_it = data.erase_and_dispose(list_it, disposer);
        return ret_it;
    }

    size_type erase(const T &v) {
        auto it = data_set.find(v, KeyValueCompare());
        if (it != data_set.end()) {
            data_set.erase(*it);
            data.erase_and_dispose(data.iterator_to(*it), disposer);
            return 1;
        }
        return 0;
    }
};

template <class T, class C1, class A1, class U>
inline auto operator|=(ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), a) {
    for (auto &el : b) a.insert(el);
    return a;
}
template <class T, class C1, class A1, class U>
inline auto operator-=(ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), a) {
    for (auto &el : b) a.erase(el);
    return a;
}
template <class T, class C1, class A1, class U>
inline auto operator&=(ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), a) {
    for (auto it = a.begin(); it != a.end();) {
        if (b.count(*it))
            ++it;
        else
            it = a.erase(it);
    }
    return a;
}

template <class T, class C1, class A1, class U>
inline auto contains(const ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), true) {
    for (auto &el : b)
        if (!a.count(el)) return false;
    return true;
}
template <class T, class C1, class A1, class U>
inline auto intersects(const ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), true) {
    for (auto &el : b)
        if (a.count(el)) return true;
    return false;
}

#endif /* _LIB_ORDERED_SET_H_ */
