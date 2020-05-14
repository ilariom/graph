# graph
A simple, header-only graph library that fits nicely with the standard library.

## Examples
A graph acts as a container for objects of the same type, exactly as std containers do. Ownership of the collected objects is not required, but can be handled by the graph.
You can easily create graph instances, as well as copy or move them.
```cpp
// graph is the basic type. Others are available, 
// but they're still just slight modifications of that one
estd::graph<std::string> G;
estd::digraph<int> DG;
estd::weighted_digraph<float, ssize_t> WDG;
estd::undirected_graph<std::vector<uint32_t>> UG;
estd::weighted_undirected_graph<std::function<void(), ssize_t> WUG;
estd::tree<int> T;

// Inserting nodes, edges or removing them is straightforward
auto hello_id = G.insert("hello");
auto world_id = G.insert("world");
auto node = G.insert("node");
G.edge(hello_id, world_id);

// Graphs are thought to be fast during iterations, but can incur
// in penalties when erases are performed. Always prefer to erase in batch
auto wrong_id = G.insert("wrong");
G.erase(wrong_id);
G.erase({ wrong_id, hello_id, world_id });

// You can inspect general properties of the graph
auto ord = G.order(); // order is the number of nodes
auto sz = G.size(); // size is the number of edges

// Or you can inspect properties of a single node
const auto& in = G.in(node); // get ingoing incident nodes
const auto& out = G.out(node); // get outgoing incident nodes
auto deg = G.degree(node); // degree is the number of incident nodes

// A tree handles some operations differently
T.append(hello_id, node); // trees use append instead of edge
auto par = T.parent(node);
const auto& children = T.children(node);

// You may check if a node has no parent by testing equality
// with the special id estd::graph<T>::null_id
if (T.parent(node) == estd::tree<std::string>::null_id)
{
   // It's the root!
}

// Finally, you can access the content with the subscript operator
auto& content = G[node]; // content == "node"
```
All graph types support three types of iterators for nodes, edges and to visit the structures. Node iterator is bidirectional, while the others are simply forward iterators.
All are const iterators that can't change a graph's content. That's needed to ensure coherence of informations, as, for instance, modifying edges directly can be dangerous.
```cpp
// Iterate through nodes
for (auto it = G.nodes_begin(); it != G.nodes_end(); ++it)
{
   // Do something with the node, for example:
   std::cout << "Node ID: " << *it << ", Node Content: " << G[*it] << std::endl;
}

// Iterate through edges
for (auto it = G.edges_begin(); it != G.edges_end(); ++it)
{
   auto pair = *it;
   auto first_node = pair.first;
   auto second_node = pair.second;
}

// Perform a visit through the graph, backed by DFS, starting from root id.
// BFS is also available
for (auto it = G.begin<estd::search_algorithm::dfs>(root); it != G.end<estd::search_algorithm::dfs>(); ++it)
{
   auto current_node = *it;
   
   // Peek next node, if it exists
   auto next_node = it.peek();

   // Prune the visit at the next node. This automatically resets after first use
   it.prune();
}

// Giving two iterators start and goal:
auto path = start > goal; // gets the shortest path from start to goal
auto dist = goal - start; // gets the shortest distance between start and goal
```

## Coming Soon
- Adds UCS, beam and A* to search algorithms
- Adds batch operator for all shortest paths from a node
- Renames iterator to search_iterator
- Moves path calculation from search_iterator to node_iterator
- Adds has_next() to search_iterator
- Adds a way to remove edges (sorry, I just forgot that :D) 
- Find a fancier name for the project

## Integration
Just copy the files wherever you need. C++11 is required. Powered by MIT license.
