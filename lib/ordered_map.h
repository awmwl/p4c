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

#ifndef LIB_ORDERED_MAP_H_
#define LIB_ORDERED_MAP_H_

#include <cassert>
#include <functional>
#include <utility>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/list.hpp>

// Map is ordered by order of element insertion.
template <class K, class V, class COMP = std::less<K>,
          class ALLOC = std::allocator<std::pair<const K, V>>>
class ordered_map {
    using list_base_hook = boost::intrusive::list_base_hook<>;
    using set_base_hook = boost::intrusive::set_base_hook<>;

    struct Node : public list_base_hook, public set_base_hook
    {
        std::pair<const K, V> data;

        template <typename ...U>
        explicit Node(U&&... u) : data(std::forward<U>(u)...) {};

        bool operator<(const Node& r) const {
            return COMP()(data.first, r.data.first);
        }
    };

 public:
    typedef K                                                   key_type;
    typedef V                                                   mapped_type;
    typedef std::pair<const K, V>                               value_type;
    typedef COMP                                                key_compare;
    typedef typename ALLOC::template rebind<Node>::other        allocator_type;
    typedef value_type                                          &reference;
    typedef const value_type                                    &const_reference;

 private:
    typedef boost::intrusive::list<Node> list_type;
    typedef boost::intrusive::set<Node> set_type;
    typedef std::allocator_traits<allocator_type> alloc_traits;

 public:
    struct iterator
        : public std::iterator<std::bidirectional_iterator_tag, std::pair<const K, V>> {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::pair<const K, V>;
        using reference = value_type&;
        using pointer = value_type*;

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

    struct const_iterator
        : public std::iterator<std::bidirectional_iterator_tag, const std::pair<const K, V>> {
        using value_type = const std::pair<const K, V>;
        using reference = value_type&;
        using pointer = value_type*;

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

    set_type                            data_map;

    struct KeyValueCompare {
        bool operator()(const Node& l, const key_type& r) const {
            return COMP()(l.data.first, r);
        }
        bool operator()(const key_type& l, const Node& r) const {
            return COMP()(l, r.data.first);
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

 public:
    // FIXME add allocator and comparator ctors...
    ordered_map() : disposer(alloc) {}
    ordered_map(std::initializer_list<value_type> init) : disposer(alloc) {
        for (const auto& e : init) {
            Node* new_item = construct(e);
            data.push_back(*new_item);
            data_set.insert(*new_item);
        }
    }
    ordered_map(const ordered_map &a) : disposer(alloc) {
        for (const auto& e : a.data) {
            Node* new_item = construct(e);
            data_set.insert(*new_item);
            data.push_back(*new_item);
        }
    }
    ordered_map(ordered_map &&a) = default;
    ordered_map &operator=(const ordered_map &a) {
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
    ordered_map &operator=(ordered_map &&a) = default;
    bool operator==(const ordered_map &a) const {
        if (data.size() != a.data.size()) return false;
        auto it_l = data.begin();
        auto it_r = a.data.begin();
        for (; it_l != data.end(); ++it_l, ++it_r) {
            if (it_l->data != it_r->data) return false;
        }
        return true;
    }
    bool operator!=(const ordered_map &a) const { return !(*this == a); }
    ~ordered_map() {
        clear();
    }

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

    bool        empty() const noexcept { return data.empty(); }
    size_type   size() const noexcept { return data_set.size(); }
    size_type   max_size() const noexcept { return data_set.max_size(); }
    void        clear() {
        data_set.clear();
        data.clear_and_dispose(disposer);
    }

    iterator        find(const key_type &a) { return tr_iter(data_set.find(a, KeyValueCompare())); }
    const_iterator  find(const key_type &a) const { return tr_iter(data_set.find(a, KeyValueCompare())); }
    size_type       count(const key_type &a) const { return data_set.count(a, KeyValueCompare()); }
    iterator        upper_bound(const key_type &a) { return tr_iter(data_set.upper_bound(a, KeyValueCompare())); }
    const_iterator  upper_bound(const key_type &a) const { return tr_iter(data_set.upper_bound(a, KeyValueCompare())); }
    iterator        lower_bound(const key_type &a) { return tr_iter(data_set.lower_bound(a, KeyValueCompare())); }
    const_iterator  lower_bound(const key_type &a) const { return tr_iter(data_set.lower_bound(a, KeyValueCompare())); }

    iterator        upper_bound_pred(const key_type &a) {
                        auto ub = data_set.upper_bound(&a);
                        if (ub == data_set.begin()) return end();
                        return tr_iter(--ub); }
    const_iterator  upper_bound_pred(const key_type &a) const {
                        auto ub = data_set.upper_bound(&a);
                        if (ub == data_set.begin()) return end();
                        return tr_iter(--ub); }

    V& operator[](const K &x) {
        auto it = find(x);
        if (it == data.end()) {
            it = emplace(x, V()).first; }
        return it->second; }
    V& operator[](K &&x) {
        auto it = find(x);
        if (it == data.end()) {
            it = emplace(std::move(x), V()).first; }
        return it->second; }
    V& at(const K &x) {
        auto it = find(x);
        if (it == end()) {
            throw std::out_of_range("ordered_map::at");
        }
        return it->second;
    }
    const V& at(const K &x) const {
        auto it = find(x);
        if (it == end()) {
            throw std::out_of_range("ordered_map::at");
        }
        return it->second;
    }

    template <typename KK, typename... VV>
    std::pair<iterator, bool> emplace(KK &&k, VV &&...v) {
        auto it = find(k);
        if (it == end()) {
            Node* new_item = construct(std::piecewise_construct_t(), std::forward_as_tuple(k),
                                  std::forward_as_tuple(std::forward<VV>(v)...));
            auto data_it = data.insert(data.end(), *new_item);
            data_set.insert(*new_item);
            return std::make_pair(data_it, true);
        }
        return std::make_pair(it, false);
    }

    template<typename KK, typename... VV>
    std::pair<iterator, bool> emplace_hint(iterator, KK &&k, VV &&... v) {
        auto it = find(k);
        if (it == end()) {
            Node* new_item = construct(std::piecewise_construct_t(), std::forward_as_tuple(k),
                                  std::forward_as_tuple(std::forward<VV>(v)...));
            auto data_it = data.insert(data.end(), *new_item);
            data_set.insert(*new_item);
            return std::make_pair(data_it, true);
        }
        return std::make_pair(it, false);
    }

    std::pair<iterator, bool> insert(const value_type &v) {
        auto it = data_set.find(v.first, KeyValueCompare());
        if (it == data_set.end()) {
            Node* new_item = construct(v);
            auto data_it = data.insert(data.end(), *new_item);
            data_set.insert(*new_item);
            return std::make_pair(iterator(data_it), true);
        }
        return std::make_pair(tr_iter(it), false);
    }

    std::pair<iterator, bool> insert(const_iterator pos, const value_type &v) {
        auto it = data_set.find(v.first, KeyValueCompare());
        if (it == data_set.end()) {
            Node* new_item = construct(v);
            auto data_it = data.insert(pos.base, *new_item);
            data_set.insert(*new_item);
            return std::make_pair(data_it, true);
        }
        return std::make_pair(tr_iter(it), false);
    }
    template<class InputIterator> void insert(InputIterator b, InputIterator e) {
        while (b != e) insert(*b++); }
    template<class InputIterator>
    void insert(const_iterator pos, InputIterator b, InputIterator e) {
        while (b != e) insert(pos, *b++); }

    iterator erase(const_iterator pos) {
        auto list_it = pos.base;
        data_set.erase(*list_it);
        auto ret_it = data.erase_and_dispose(list_it, disposer);
        return ret_it;
    }

    size_type erase(const K &k) {
        auto it = data_set.find(k, KeyValueCompare());
        if (it != data_set.end()) {
            data_set.erase(*it);
            data.erase_and_dispose(data.iterator_to(*it), disposer);
            return 1;
        }
        return 0;
    }

    template<class Compare> void sort(Compare comp) {
        auto node_comp = [&comp] (const Node& l, const Node& r) {
            return comp(l.data, r.data);
        };
        data.sort(node_comp);
    }
};

// XXX(seth): We use this namespace to hide our get() overloads from ADL. GCC
// 4.8 has a bug which causes these overloads to be considered when get() is
// called on a type in the global namespace, even if the number of arguments
// doesn't match up, which can trigger template instantiations that cause
// errors.
namespace GetImpl {

template <class K, class T, class V, class Comp, class Alloc>
inline V get(const ordered_map<K, V, Comp, Alloc> &m, T key, V def = V()) {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    return def;
}

template <class K, class T, class V, class Comp, class Alloc>
inline V *getref(ordered_map<K, V, Comp, Alloc> &m, T key) {
    auto it = m.find(key);
    if (it != m.end()) return &it->second;
    return 0;
}

template <class K, class T, class V, class Comp, class Alloc>
inline const V *getref(const ordered_map<K, V, Comp, Alloc> &m, T key) {
    auto it = m.find(key);
    if (it != m.end()) return &it->second;
    return 0;
}

template <class K, class T, class V, class Comp, class Alloc>
inline V get(const ordered_map<K, V, Comp, Alloc> *m, T key, V def = V()) {
    return m ? get(*m, key, def) : def;
}

template <class K, class T, class V, class Comp, class Alloc>
inline V *getref(ordered_map<K, V, Comp, Alloc> *m, T key) {
    return m ? getref(*m, key) : 0;
}

template <class K, class T, class V, class Comp, class Alloc>
inline const V *getref(const ordered_map<K, V, Comp, Alloc> *m, T key) {
    return m ? getref(*m, key) : 0;
}

}  // namespace GetImpl
using namespace GetImpl;  // NOLINT(build/namespaces)

#endif /* LIB_ORDERED_MAP_H_ */
