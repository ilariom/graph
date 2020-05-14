template<typename T, typename V>
class graph;

namespace search_algorithm
{

struct graph_setter
{
public:
    template<typename T, typename V>
    void set_graph(const graph<T, V>&) { }
};

template<typename container_type>
struct graph_container : public container_type, public graph_setter
{ };

template<>
struct graph_container<std::queue<size_t>> : public std::queue<size_t>, public graph_setter
{
public:
    size_t top() const { return front(); }
};

using dfs = graph_container<std::stack<size_t>>;
using bfs = graph_container<std::queue<size_t>>;

} // namespace search_algorithm