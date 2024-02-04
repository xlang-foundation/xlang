#include "graph.h"

#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
	boost::no_property, boost::property<boost::edge_weight_t, int>> Graph;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
using boost::add_edge;


namespace X
{
	namespace Algorithm
	{
		XGraph::XGraph()
		{
			mGraph = (void*)new Graph();
#if false
			Graph& g = *(Graph*)mGraph;

			add_edge(0, 1, 2, g);
			add_edge(1, 2, 3, g);
			add_edge(2, 3, 5, g);
			add_edge(3, 4, 2, g);
			add_edge(4, 0, 4, g);

			std::vector<Edge> spanning_tree;
			kruskal_minimum_spanning_tree(g, std::back_inserter(spanning_tree));
			for (std::vector<Edge>::iterator ei = spanning_tree.begin(); ei != spanning_tree.end(); ++ei) 
			{
				std::cout << source(*ei, g) << " <--> " << target(*ei, g) << " with weight of " << get(boost::edge_weight, g, *ei) << std::endl;
			}
			// Select a node to act as the 'root' for BFS
			Vertex root = 0; // Change this if you want a different root
			std::cout << "\nPerforming BFS starting from node " << root << std::endl;
			std::vector<Vertex> p(num_vertices(g));
			prim_minimum_spanning_tree(g, &p[0], boost::root_vertex(root));

			for (std::size_t i = 0; i != p.size(); ++i) 
			{
				if (p[i] != i) 
				{
					std::cout << "Parent of " << i << " is " << p[i] << std::endl;
				}
				else 
				{
					std::cout << i << " is the root." << std::endl;
				}
			}
#endif
		}
		bool XGraph::AddEdge(int id1, int id2, double weight)
		{
			Graph& g = *(Graph*)mGraph;
			add_edge(id1, id2, weight,g);
			return true;
		}
		X::Value XGraph::Prim_MST(long long root)
		{
			Graph& g = *(Graph*)mGraph;
			std::vector<Vertex> p(num_vertices(g));
			prim_minimum_spanning_tree(g, &p[0], boost::root_vertex(root));
			X::Tensor t;
			t->SetDataType(X::TensorDataType::LONGLONG);
			Port::vector<int> shapes(1);
			shapes.push_back(p.size());
			t->SetShape(shapes);
			X::Value initData;
			t->Create(initData);
			t->SetData((char*)p.data(), p.size()*sizeof(Vertex));
			return t;
		}
	}
}