template <typename T, typename V>
inline typename graph<T, V>::id_type graph<T, V>::insert(
    typename std::conditional<std::is_arithmetic<value_type>::value, value_type, 
    const value_type&>::type val
)
{
    if (!invalid_nodes_.empty())
    {
        auto it = invalid_nodes_.cbegin();
        auto node = *it;
        invalid_nodes_.erase(it);
        objs_[node] = val;
        removed_nodes_--;

        return node;
    }

    adjs_.emplace_back();
    radjs_.emplace_back();
    ws_.emplace_back();
    rws_.emplace_back();
    objs_.push_back(val);
    
    return objs_.size() - 1;
}

template <typename T, typename V>
inline void graph<T, V>::erase(id_type node)
{
    erase(nodes_container { node });
}

template <typename T, typename V>
inline void graph<T, V>::erase(const nodes_container& nodes)
{
    auto repop_nodes = [] (std::vector<nodes_container>& dst, const nodes_container& nodes) mutable {
        for (auto node = 0; node < dst.size(); ++node)
        {
            if (std::find(nodes.begin(), nodes.end(), node) != nodes.end())
            {
                dst[node].clear();
            }
        }
        
        for (auto k = 0; k < dst.size(); ++k)
        {
            nodes_container& src = dst[k];
            nodes_container tmp;

            for (auto node : src)
            {
                if (std::find(nodes.begin(), nodes.end(), node) != nodes.end())
                {
                    continue;
                }

                tmp.push_back(node);
            }

            src = std::move(tmp);
        }
    };

    auto repop_weights = [] (std::vector<std::unordered_map<id_type, weight_type>>& dst, const nodes_container& nodes) mutable {
        for (auto node = 0; node < dst.size(); ++node)
        {
            if (std::find(nodes.begin(), nodes.end(), node) != nodes.end())
            {
                dst[node].clear();
            }
        }

        for (auto k = 0; k < dst.size(); ++k)
        {
            std::unordered_map<id_type, weight_type>& src = dst[k];
            std::unordered_map<id_type, weight_type> tmp;

            for (auto& pair : src)
            {
                auto node = pair.first;

                if (std::find(nodes.begin(), nodes.end(), node) != nodes.end())
                {
                    continue;
                }

                tmp.insert(pair);
            }

            src = std::move(tmp);
        }
    };

    size_type sz = order();

    for (auto node = 0; node < objs_.size(); ++node)
    {
        if (std::find(nodes.begin(), nodes.end(), node) != nodes.end())
        {
            objs_[node] = value_type {};
            sz--;
        }
    }

    removed_nodes_ += order() - sz;

    repop_nodes(adjs_, nodes);
    repop_nodes(radjs_, nodes);
    repop_weights(ws_, nodes);
    repop_weights(rws_, nodes);

    for (auto node : nodes)
    {
        invalid_nodes_.insert(node);
    }
}

template <typename T, typename V>
void graph<T, V>::erase(id_type first, id_type second)
{
    auto it = std::find(adjs_[first].begin(), adjs_[first].end(), second);

    if (it != adjs_[first].end())
    {
        adjs_[first].erase(it);
    }
    
    auto rit = std::find(radjs_[second].begin(), radjs_[second].end(), first);

    if (rit != radjs_[second].end())
    {
        radjs_[second].erase(rit);
    }
}

template <typename T, typename V>
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

template <typename T, typename V>
inline typename graph<T, V>::weight_type graph<T, V>::weight(id_type node, id_type child) const
{
    return node < ws_.size() && ws_[node].find(child) != ws_[node].end() ?
        ws_[node].at(child)
        : std::numeric_limits<graph<T, V>::weight_type>::max()
    ;
}

template <typename T, typename V>
inline size_t graph<T, V>::size() const
{
    size_t t = 0;
    
    for (id_type node = 0; node < adjs_.size(); ++node)
    {
        t += adjs_[node].size();
    }
    
    return t;
}

template <typename T, typename V>
template <typename container_type>
void graph<T, V>::search_iterator<container_type>::rewind()
{
    curr_ = root_;
    frontier_.clear();
    E_.clear();
    prune_ = false;
    step();
}

template <typename T, typename V>
template <typename container_type>
inline void graph<T, V>::search_iterator<container_type>::step()
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

template <typename T, typename V>
template <typename container_type>
inline typename graph<T, V>::template search_iterator<container_type>& graph<T, V>::search_iterator<container_type>::operator++()
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

template <typename T, typename V>
template <typename container_type>
inline typename graph<T, V>::weight_type graph<T, V>::search_iterator<container_type>::operator-(const search_iterator& other) const
{
    return (other > G_).distance_to(curr_);
}

template <typename T, typename V>
template <typename container_type>
inline typename graph<T, V>::nodes_container graph<T, V>::search_iterator<container_type>::operator<(const search_iterator& other) const
{
    if (*other == graph<T, V>::null_id)
    {
        return {};
    }

    auto path = other > G_;
    return path.path_to(curr_);
}

template <typename T, typename V>
template <typename container_type>
inline typename graph<T, V>::path graph<T, V>::search_iterator<container_type>::operator>(const graph<T, V>& G) const
{
    if (curr_ == graph<T, V>::null_id)
    {
        return {};
    }
    
    if (G_.is_weighted())
    {
        return bellman_ford(G_, curr_);
    }

    return bfs_distance(G_, curr_);
}

#define NODE_ITER_OP(slide) while (!G_.is_valid(v_) && v_ < G_.order()) slide; if (v_ >= G_.order()) v_ = graph<T, V>::null_id; return *this

template <typename T, typename V>
inline typename graph<T, V>::node_iterator& graph<T, V>::node_iterator::operator++()
{
    ++v_;
    NODE_ITER_OP(++v_);
}

template <typename T, typename V>
inline typename graph<T, V>::node_iterator& graph<T, V>::node_iterator::operator--()
{
    --v_;
    NODE_ITER_OP(--v_);
}

template <typename T, typename V>
inline typename graph<T, V>::node_iterator& graph<T, V>::node_iterator::operator+(size_t n)
{
    v_ += n;
    NODE_ITER_OP(++v_);
}

template <typename T, typename V>
inline typename graph<T, V>::node_iterator& graph<T, V>::node_iterator::operator-(size_t n)
{
    v_ -= n;
    NODE_ITER_OP(++v_);
}

#undef NODE_ITER_OP

template <typename T, typename V>
inline bool graph<T, V>::edge_iterator::ensure_validity()
{
    if (it_ == G_.nodes_end())
    {
        u_ = graph<T, V>::null_id;
        v_ = graph<T, V>::null_id;
        return false;
    }

    return true;
}

template <typename T, typename V>
inline typename graph<T, V>::edge_iterator& graph<T, V>::edge_iterator::operator++()
{
    if (!ensure_validity())
    {
        return *this;
    }

    if (u_ != graph<T, V>::null_id && adjs_idx_ < G_.out(u_).size())
    {
        v_ = G_.out(u_)[adjs_idx_++];
    }
    else
    {
        u_ = *it_;

        while (G_.out(u_).empty())
        {
            ++it_;

            if (!ensure_validity())
            {
                return *this;
            }

            u_ = *it_;
        }

        adjs_idx_ = 0;
        v_ = G_.out(u_)[adjs_idx_++];

        ++it_;
    }
    
    return *this;
}

template <typename T, typename V>
inline typename graph<T, V>::path_array graph<T, V>::path::path_to(typename graph<T, V>::id_type node) const
{
    typename graph<T, V>::path_array p;
    typename graph<T, V>::id_type v = node;
        
    while (v != graph<T, V>::null_id)
    {
        p.push_back(v);
        v = parents_[v];

        if (v == node)
        {
            break;
        }
    }

    std::reverse(p.begin(), p.end());
    return p;
}

template <typename T, typename V>
inline typename graph<T, V>::path bfs_distance(const graph<T, V>& G, typename graph<T, V>::id_type root)
{
    std::vector<typename graph<T, V>::weight_type> level(G.order());
    std::vector<typename graph<T, V>::id_type> p(G.order());

    level[root] = 0;
    p[root] = graph<T, V>::null_id;

    for (
        auto it = G.template begin<estd::search_algorithm::bfs>(root); 
        it != G.template end<estd::search_algorithm::bfs>(); 
        ++it
    )
        for (typename graph<T, V>::id_type child : G.out(*it))
        {
            level[child] = level[*it] + 1;
            p[child] = *it;
        }

    return typename graph<T, V>::path {
        std::move(p),
        std::move(level),
        root
    };
}

template <typename T, typename V>
inline typename graph<T, V>::path bellman_ford(const graph<T, V>& G, typename graph<T, V>::id_type root)
{
    std::vector<typename graph<T, V>::weight_type> d(G.order());
    std::vector<typename graph<T, V>::id_type> p(G.order());

    std::fill_n(d.begin(), G.order(), std::numeric_limits<typename graph<T, V>::weight_type>::max());
    typename graph<T, V>::id_type null_id = graph<T, V>::null_id;
    std::fill_n(p.begin(), G.order(), null_id);

    d[root] = 0;

    for (auto bfstep = 1; bfstep < G.order(); ++bfstep)
        for (auto edge = G.edges_begin(); edge != G.edges_end(); ++edge)
        {
            auto u = (*edge).first;
            auto v = (*edge).second;

            if (d[u] + G.weight(u, v) < d[v])
            {
                d[v] = d[u] + G.weight(u, v);
                p[v] = u;
            }
        }

    return typename graph<T, V>::path {
        std::move(p),
        std::move(d),
        root
    };
}