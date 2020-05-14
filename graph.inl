template<typename T, typename V>
inline typename graph<T, V>::id_type graph<T, V>::insert(
    typename std::conditional<std::is_arithmetic<value_type>::value, value_type, 
    const value_type&>::type val
)
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
    erase(nodes_container { node });
}

template<typename T, typename V>
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

    removed_nodes_ = order() - sz;

    repop_nodes(adjs_, nodes);
    repop_nodes(radjs_, nodes);
    repop_weights(ws_, nodes);
    repop_weights(rws_, nodes);

    for (auto node : nodes)
    {
        invalid_nodes_.insert(node);
    }
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
    prune_ = false;
    step();
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
inline typename graph<T, V>::nodes_container graph<T, V>::iterator<container_type>::operator<(const iterator& other) const
{
    if (other.curr_ == graph<T, V>::null_id || curr_ == graph<T, V>::null_id)
    {
        return {};
    }
    
    graph<T, V>::nodes_container path;
    
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
            for (auto edge = G_.edges_begin(); edge != G_.edges_end(); ++edge)
                {
                    auto u = (*edge).first;
                    auto v = (*edge).second;
                    
                    if (distance[u] + G_.weight(u, v) < distance[v])
                    {
                        distance[v] = distance[u] + G_.weight(u, v);
                        predecessor[v] = u;
                    }
                }
                
        for (auto edge = G_.edges_begin(); edge != G_.edges_end(); ++edge)
        {
            auto u = (*edge).first;
            auto v = (*edge).second;
            
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
inline typename graph<T, V>::node_iterator& graph<T, V>::node_iterator::operator++()
{
     ++v_;
    while (!G_.is_valid(v_)) ++v_;

    if (v_ >= G_.order())
    {
        v_ = graph<T, V>::null_id;
    }

    return *this;
}

template<typename T, typename V>
inline typename graph<T, V>::node_iterator& graph<T, V>::node_iterator::operator--()
{
    --v_;
    while (!G_.is_valid(v_)) --v_;
    
    if (v_ >= G_.order())
    {
        v_ = graph<T, V>::null_id;
    }

    return *this;
}

template<typename T, typename V>
inline typename graph<T, V>::node_iterator& graph<T, V>::node_iterator::operator+(size_t n)
{
    v_ += n;
    while (!G_.is_valid(v_)) ++v_;

    if (v_ >= G_.order())
    {
        v_ = graph<T, V>::null_id;
    }

    return *this;
}

template<typename T, typename V>
inline typename graph<T, V>::node_iterator& graph<T, V>::node_iterator::operator-(size_t n)
{
    v_ -= n;
    while (!G_.is_valid(v_)) ++v_;

    if (v_ >= G_.order())
    {
        v_ = graph<T, V>::null_id;
    }

    return *this;
}

template<typename T, typename V>
inline typename graph<T, V>::edge_iterator& graph<T, V>::edge_iterator::operator++()
{
    if (it_ == G_.nodes_end())
    {
        u_ = graph<T, V>::null_id;
        v_ = graph<T, V>::null_id;
        return *this;
    }

    if (u_ != graph<T, V>::null_id && adjs_idx_ < G_.out(u_).size())
    {
        v_ = G_.out(u_)[adjs_idx_++];
    }
    else
    {
        u_ = *it_;
        adjs_idx_ = 0;
        v_ = G_.out(u_)[adjs_idx_++];

        ++it_;
    }
    
    return *this;
}
