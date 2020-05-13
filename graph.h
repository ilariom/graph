#ifndef graph_h
#define graph_h

#include <vector>
#include <unordered_set>
#include <stack>
#include <queue>
#include <limits>
#include <algorithm>

namespace estd
{

namespace search_algorithm
{
template<typename container_type>
struct graph_container : public container_type
{ };

template<>
struct graph_container<std::queue<size_t>> : public std::queue<size_t>
{
public:
    size_t top() const { return front(); }
};

using dfs = graph_container<std::stack<size_t>>;
using bfs = graph_container<std::queue<size_t>>;

} // namespace search_algorithm

template<typename T, typename V = ssize_t>
class graph
{
public:
    using value_type = T;
    using weight_type = V;
    using size_type = size_t;
    using id_type = size_t;
    
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
            frontier_.push(root);
            step();
        }
        
        iterator(const iterator& it)
            : G_(it.G_), children_(it.children_), curr_(it.curr_), root_(it.root_)
        {
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
    
    using edge_type = std::pair<graph<T, V>::id_type, graph<T, V>::id_type>;
    
    class edge_iterator
    {
    public:
        edge_iterator(const graph<T, V>& G, graph<T, V>::id_type u = graph<T, V>::null_id)
            : G_(G), u_(u)
        { }
        
    public:
        edge_type operator*() const { return { u_, v_ }; }
        edge_iterator& operator++();
        
        bool operator==(const edge_iterator& other) const { return u_ == other.u_ && v_ == other.v_; }
        bool operator!=(const edge_iterator& other) const { return !(*this == other); }
        
    private:
        const graph<T, V>& G_;
        graph<T, V>::id_type u_;
        graph<T, V>::id_type v_ = 0;
    };
    
public:
    id_type insert(typename std::conditional<std::is_arithmetic<T>::value, T, const T&>::type);
    void erase(size_t);
    size_type degree(id_type node) const { return in(node).size() + out(node).size(); }
    size_type order() const { return objs_.size(); }
    size_type size() const;
    bool empty() const { return order() == 0; }
    
    const std::vector<id_type>& in(id_type node) const { return radjs_[node]; }
    const std::vector<id_type>& out(id_type node) const { return adjs_[node]; }
    
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
    
private:
    std::vector<std::vector<id_type>> adjs_;
    std::vector<std::vector<id_type>> radjs_;
    std::vector<std::unordered_map<id_type, weight_type>> ws_;
    std::vector<std::unordered_map<id_type, weight_type>> rws_;
    std::vector<T> objs_;
    bool weighted_ = false;
};

template<typename T, typename V>
inline typename graph<T, V>::id_type graph<T, V>::insert(typename std::conditional<std::is_arithmetic<T>::value, T, const T&>::type val)
{
    adjs_.emplace_back();
    radjs_.emplace_back();
    ws_.emplace_back();
    rws_.emplace_back();
    objs_.push_back(val);
    
    return objs_.size() - 1;
}

template<typename T, typename V>
inline void graph<T, V>::erase(id_type node)
{
    for (id_type par : in(node))
    {
        auto it = std::find(adjs_[par].begin(), adjs_[par].end(), node);
        
        if (it != adjs_[par].end())
        {
            adjs_[par].erase(it);
        }
    }
    
    for (id_type child : out(node))
    {
        auto it = std::find(radjs_[child].begin(), radjs_[child].end(), node);
        
        if (it != radjs_[child].end())
        {
            radjs_[child].erase(it);
        }
    }
    
    objs_.erase(objs_.begin() + node);
    adjs_.erase(adjs_.begin() + node);
    radjs_.erase(radjs_.begin() + node);
}

template<typename T, typename V>
inline void graph<T, V>::edge(id_type node, id_type child, weight_type w)
{
    adjs_[node].push_back(child);
    radjs_[child].push_back(node);
    ws_[node][child] = w;
    rws_[child][node] = w;
    
    if (w != 1)
    {
        weighted_ = true;
    }
}

template<typename T, typename V>
inline typename graph<T, V>::weight_type graph<T, V>::weight(id_type node, id_type child) const
{
    return node < ws_.size() && ws_[node].find(child) != ws_[node].end() ?
        ws_[node].at(child)
        : std::numeric_limits<graph<T, V>::weight_type>::max()
    ;
}

template<typename T, typename V>
inline size_t graph<T, V>::size() const
{
    size_t t = 0;
    
    for (id_type node = 0; node < adjs_.size(); ++node)
    {
        t += adjs_[node].size();
    }
    
    return t;
}

template<typename T, typename V>
template<typename container_type>
void graph<T, V>::iterator<container_type>::rewind()
{
    curr_ = root_;
    frontier_.clear();
    E_.clear();
    step();
    prune_ = false;
}

template<typename T, typename V>
template<typename container_type>
inline void graph<T, V>::iterator<container_type>::step()
{
    curr_ = frontier_.top();
    frontier_.pop();
    E_.insert(curr_);
    
    if (curr_ != graph<T, V>::null_id && !prune_)
    {
        for (id_type child : children_[curr_])
        {
            if (E_.find(child) != E_.end())
            {
                continue;
            }
            
            frontier_.push(child);
        }
    }
    
    prune_ = false;
}

template<typename T, typename V>
template<typename container_type>
inline typename graph<T, V>::template iterator<container_type>& graph<T, V>::iterator<container_type>::operator++()
{
    if (frontier_.empty())
    {
        curr_ = graph<T, V>::null_id;
    }
    else
    {
        step();
    }
    
    return *this;
}

template<typename T, typename V>
template<typename container_type>
inline typename graph<T, V>::weight_type graph<T, V>::iterator<container_type>::operator-(const iterator& other) const
{
    auto v = *this < other;
    graph<T, V>::weight_type len = 0;
    
    for (auto k = 1; k < v.size(); ++k)
    {
        graph<T, V>::id_type node = v[k - 1];
        graph<T, V>::id_type child = v[k];
        
        auto w = G_.weight(node, child);
        
        if (w == std::numeric_limits<graph<T, V>::weight_type>::max())
        {
            return w;
        }
        
        len += w;
    }
    
    return len;
}

template<typename T, typename V>
template<typename container_type>
inline std::vector<typename graph<T, V>::id_type> graph<T, V>::iterator<container_type>::operator<(const iterator& other) const
{
    if (other.curr_ == graph<T, V>::null_id || curr_ == graph<T, V>::null_id)
    {
        return {};
    }
    
    std::vector<typename graph<T, V>::id_type> path;
    
    if (!G_.is_weighted())
    {
        std::unordered_map<graph<T, V>::id_type, graph<T, V>::id_type> p;
        std::unordered_map<graph<T, V>::id_type, size_t> level;
        level[other.curr_] = 0;
        p[other.curr_] = graph<T, V>::null_id;
        
        for (auto it = G_.begin<search_algorithm::bfs>(other.curr_); it != G_.end<search_algorithm::bfs>(); ++it)
            for (graph<T, V>::id_type child : G_.out(*it))
            {
                level[child] = level[*it] + 1;
                p[child] = *it;
            }
        
        if (p.find(curr_) != p.end())
        {
            graph<T, V>::id_type v = curr_;
            
            while (v != graph<T, V>::null_id && v != other.curr_)
            {
                path.push_back(v);
                v = p[v];
            }
            
            if (!path.empty())
            {
                path.push_back(other.curr_);
            }
            
            std::reverse(path.begin(), path.end());
        }
    }
    else
    {
        std::vector<graph<T, V>::weight_type> distance(G_.order());
        std::vector<graph<T, V>::id_type> predecessor(G_.order());
        
        std::fill_n(distance.begin(), G_.order(), std::numeric_limits<graph<T, V>::weight_type>::max());
        graph<T, V>::id_type null_id = graph<T, V>::null_id;
        std::fill_n(predecessor.begin(), G_.order(), null_id);
        
        distance[other.curr_] = 0;
        
        for (auto bfstep = 1; bfstep < G_.order(); ++bfstep)
            for (auto u = 0; u < G_.order(); ++u)
                for (auto edgeIdx = 0; edgeIdx < G_.out(u).size(); ++edgeIdx)
                {
                    auto v = G_.out(u)[edgeIdx];
                    
                    if (distance[u] + G_.weight(u, v) < distance[v])
                    {
                        distance[v] = distance[u] + G_.weight(u, v);
                        predecessor[v] = u;
                    }
                }
                
        for (auto u = 0; u < G_.order(); ++u)
            for (auto edgeIdx = 0; edgeIdx < G_.out(u).size(); ++edgeIdx)
            {
                auto v = G_.out(u)[edgeIdx];
                
                if (distance[u] + G_.weight(u, v) < distance[v])
                {
                    return path;
                }
            }
            
        graph<T, V>::id_type v = curr_;
        
        while (v != graph<T, V>::null_id && v != other.curr_)
        {
            path.push_back(v);
            v = predecessor[v];
        }
        
        if (!path.empty())
        {
            path.push_back(other.curr_);
        }
        
        std::reverse(path.begin(), path.end());
    }
    
    if (!path.empty() && path.back() != curr_)
    {
        path.clear();
    }
    
    return path;
}

template<typename T, typename V>
inline typename graph<T, V>::edge_iterator& graph<T, V>::edge_iterator::operator++()
{
    ++u_;
    ++v_;
    
    if (v_ >= G_.order())
    {
        v_ = 0;
    }
    
    if (u_ >= G_.order())
    {
        u_ = graph<T, V>::null_id;
    }
    
    return *this;
}

} // namespace estd

#endif /* graph_h */
