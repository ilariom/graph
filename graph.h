#ifndef graph_h
#define graph_h

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <limits>
#include <algorithm>

namespace estd
{

#include "search_algorithm.inl"

template<typename T, typename V = ssize_t>
class graph
{
public:
    using value_type = T;
    using weight_type = V;
    using size_type = size_t;
    using id_type = size_t;
    using nodes_container = std::vector<id_type>;
    
    static constexpr const id_type null_id = std::numeric_limits<id_type>::max();
    
public:
    template<typename container_type>
    class iterator
    {
    public:
        iterator(
            const graph<T, V>& G,
            const std::vector<std::vector<graph<T, V>::id_type>>& children,
            graph<T, V>::id_type root = graph<T, V>::null_id
        ) : G_(G), children_(children), curr_(root), root_(root)
        {
            frontier_.set_graph(G_);
            frontier_.push(root);
            step();
        }
        
        iterator(const iterator& it)
            : G_(it.G_), children_(it.children_), curr_(it.curr_), root_(it.root_)
        {
            frontier_.set_graph(G_);
            frontier_.push(curr_);
            step();
        }
        
    public:
        graph<T, V>::id_type operator*() const { return curr_; }
        iterator& operator++();
        graph<T, V>::weight_type operator-(const iterator& other) const;
        std::vector<graph<T, V>::id_type> operator<(const iterator& other) const;
        std::vector<graph<T, V>::id_type> operator>(const iterator& other) const { return other < *this; }
        
        bool operator==(const iterator& other) const { return curr_ == other.curr_; }
        bool operator!=(const iterator& other) const { return !(*this == other); }
        
        void prune() { prune_ = true; }
        void rewind();
        
        graph<T, V>::id_type peek() const { return frontier_.top(); }
        
    private:
        void step();
        
    private:
        const graph<T, V>& G_;
        std::unordered_set<id_type> E_;
        container_type frontier_;
        const std::vector<std::vector<graph<T, V>::id_type>>& children_;
        graph<T, V>::id_type curr_;
        graph<T, V>::id_type root_;
        bool prune_ = false;
    };

    class node_iterator
    {
    public:
        node_iterator(const graph<T, V>& G, graph<T, V>::id_type v = graph<T, V>::null_id)
            : G_(G), v_(v)
        { }
        
    public:
        id_type operator*() const { return v_; }
        
        node_iterator& operator++();
        node_iterator& operator--();
        node_iterator& operator+(size_t n);
        node_iterator& operator-(size_t n);

        bool operator==(const node_iterator& other) const { return v_ == other.v_; }
        bool operator!=(const node_iterator& other) const { return !(*this == other); }
        
    private:
        const graph<T, V>& G_;
        graph<T, V>::id_type v_;
    };

    using edge_type = std::pair<graph<T, V>::id_type, graph<T, V>::id_type>;
    
    class edge_iterator
    {
    public:
        edge_iterator(const graph<T, V>& G, graph<T, V>::id_type u = graph<T, V>::null_id)
            : G_(G), it_{ G_, u }
        {
            ++*this;
        }
        
    public:
        edge_type operator*() const { return { u_, v_ }; }
        edge_iterator& operator++();
        
        bool operator==(const edge_iterator& other) const { return u_ == other.u_ && v_ == other.v_; }
        bool operator!=(const edge_iterator& other) const { return !(*this == other); }
        
    private:
        const graph<T, V>& G_;
        node_iterator it_;
        size_t adjs_idx_ = 0;
        id_type u_ = graph<T, V>::null_id;
        id_type v_ = graph<T, V>::null_id;
    };
    
public:
    id_type insert(typename std::conditional<std::is_arithmetic<value_type>::value, value_type, const value_type&>::type);
    void erase(id_type);
    void erase(const nodes_container&);
    size_type degree(id_type node) const { return in(node).size() + out(node).size(); }
    size_type order() const { return objs_.size() - removed_nodes_; }
    size_type size() const;
    bool empty() const { return order() == 0; }
    
    const nodes_container& in(id_type node) const { return radjs_[node]; }
    const nodes_container& out(id_type node) const { return adjs_[node]; }
    
    T& operator[](id_type node) { return objs_[node]; }
    const T& operator[](id_type node) const { return objs_[node]; }
    
    template<typename search_algorithm>
    iterator<search_algorithm> begin(id_type root) const { return iterator<search_algorithm> { *this, adjs_, root }; }
    
    template<typename search_algorithm>
    iterator<search_algorithm> end() const { return iterator<search_algorithm> { *this, adjs_ }; }
    
    template<typename search_algorithm>
    iterator<search_algorithm> rbegin(id_type root) const { return iterator<search_algorithm> { *this, radjs_, root }; }
    
    template<typename search_algorithm>
    iterator<search_algorithm> rend() const { return iterator<search_algorithm> { *this, radjs_ }; }
    
    void edge(id_type node, id_type child, weight_type w = 1);
    weight_type weight(id_type node, id_type child) const;
    bool is_weighted() const { return weighted_; }
    
    edge_iterator edges_begin() const { return edge_iterator { *this, 0 }; }
    edge_iterator edges_end() const { return edge_iterator { *this }; }

    node_iterator nodes_begin() const { return node_iterator { *this, 0 }; }
    node_iterator nodes_end() const { return node_iterator { *this }; }

    bool is_valid(id_type node) const { return invalid_nodes_.find(node) == invalid_nodes_.end(); }
    
private:
    std::vector<nodes_container> adjs_;
    std::vector<nodes_container> radjs_;
    std::vector<std::unordered_map<id_type, weight_type>> ws_;
    std::vector<std::unordered_map<id_type, weight_type>> rws_;
    std::vector<value_type> objs_;
    std::unordered_set<id_type> invalid_nodes_;
    size_type removed_nodes_ = 0;
    bool weighted_ = false;
};

template<typename T, typename V>
class undirected_graph : public graph<T, V>
{
public:
    using value_type = typename graph<T, V>::value_type;
    using weight_type = typename graph<T, V>::weight_type;
    using size_type = typename graph<T, V>::size_type;
    using id_type = typename graph<T, V>::id_type;
    using nodes_container = typename graph<T, V>::nodes_container;

public:
    void edge(undirected_graph::id_type node, undirected_graph::id_type o, undirected_graph::weight_type w = 1)
    {
        graph<T, V>::edge(node, o, w);
        graph<T, V>::edge(o, node, w);
    }
};

template<typename value_type, typename weight_type>
using weighted_digraph = graph<value_type, weight_type>;

template<typename value_type>
using digraph = graph<value_type>;

template<typename value_type, typename weight_type>
using weighted_undirected_graph = undirected_graph<value_type, weight_type>;

template<typename T>
class tree : private graph<T>
{
public:
    using value_type = typename graph<T>::value_type;
    using size_type = typename graph<T>::size_type;
    using id_type = typename graph<T>::id_type;
    using nodes_container = typename graph<T>::nodes_container;

    using graph<T>::null_id;

public:
    size_type order() const { return graph<T>::order(); }
    size_type size() const { return order() > 0 ? order() - 1 : 0; }

    id_type parent(id_type node) const { return graph<T>::in(node).size() > 0 ? graph<T>::out(node)[0] : tree<T>::null_id; }
    const nodes_container& children(id_type node) const { return graph<T>::out(node); }
    void append(id_type node, id_type child) { graph<T>::edge(node, child); };

    using graph<T>::insert;
    using graph<T>::erase;
    using graph<T>::edges_begin;
    using graph<T>::edges_end;
    using graph<T>::nodes_begin;
    using graph<T>::nodes_end;

    using graph<T>::begin;
    using graph<T>::end;

    using graph<T>::operator[];
    using graph<T>::is_valid;
};

#include "graph.inl"

} // namespace estd

#endif /* graph_h */
