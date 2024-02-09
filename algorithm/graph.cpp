#include "graph.h"

#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>
#include <unordered_map>

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
	
		static std::vector<long long> ValueToArray(X::Value valAry)
		{
			X::Tensor tensor(valAry);
			long long arySize = tensor->GetDimSize(0);
			long long* t = (long long*)tensor->GetData();
			std::vector<long long> ary(t, t + arySize);
			return ary;
		}
		static X::Value ArrayToValue(const std::vector<long long>& ary)
		{
			X::Tensor valArray;
			valArray->SetDataType(X::TensorDataType::LONGLONG);
			Port::vector<int> shapes(1);
			shapes.push_back(ary.size());
			valArray->SetShape(shapes);
			X::Value initData;
			valArray->Create(initData);
			valArray->SetData((char*)ary.data(), ary.size() * sizeof(long long));
			return valArray;
		}
		X::Value XGraph::Prim_MST(long long root)
		{
			Graph& g = *(Graph*)mGraph;
			std::vector<Vertex> p(num_vertices(g));
			prim_minimum_spanning_tree(g, &p[0], boost::root_vertex(root));
			return ArrayToValue(std::vector<long long>(p.begin(), p.end()));
		}
		//tree is a tensor which's data hold the Parent-Tree array
		X::Value XGraph::PTreeChildren(X::Value tree, long long node)
		{
			std::vector<long long> aryTree = ValueToArray(tree);

			std::vector<long long> children;

			// Iterate through the aryTree
			for (uint16_t i = 0; i < aryTree.size(); ++i)
			{
				if (aryTree[i] == node && i != node) 
				{ 
					children.push_back(i);
				}
			}
			return ArrayToValue(std::vector<long long>(children.begin(), children.end()));
		}

		//In a parent array (binary_tree_representation) 
		// where each element at index i points to the parent of node i, 
		// we need a way to find the children of any given node to 
		// traverse downwards in the tree.


		template <typename INDEX_TYPE>
		void extractSubtree(const std::vector<INDEX_TYPE>& parent_array, 
			INDEX_TYPE subtree_root, std::vector<INDEX_TYPE>& subtree_parent_array, 
			std::vector<INDEX_TYPE>& new_to_old_index) 
		{
			std::vector<bool> visited(parent_array.size(), false);
			std::vector<INDEX_TYPE> old_to_new_index(parent_array.size(),0);
			std::queue<INDEX_TYPE> q;

			// Start from the subtree root
			q.push(subtree_root);
			visited[subtree_root] = true;
			old_to_new_index[subtree_root] = 0;
			new_to_old_index.push_back(subtree_root);

			while (!q.empty()) 
			{
				INDEX_TYPE current = q.front();
				q.pop();

				// Explore the children of 'current'
				for (INDEX_TYPE i = 0; i < parent_array.size(); ++i) 
				{
					if (parent_array[i] == current && !visited[i]) 
					{
						visited[i] = true;
						q.push(i);

						// Map from old to new index for the child
						old_to_new_index[i] = new_to_old_index.size();
						new_to_old_index.push_back(i);
					}
				}
			}

			// Now, build the subtree_parent_array using the old_to_new_index mapping
			subtree_parent_array.resize(new_to_old_index.size(), 0);
			for (INDEX_TYPE i = 0; i < new_to_old_index.size(); ++i) 
			{
				INDEX_TYPE old_index = new_to_old_index[i];
				INDEX_TYPE old_parent_index = parent_array[old_index];

				// Check if the node is not the root of the subtree and its parent is part of the subtree
				if (old_index != subtree_root) 
				{
					subtree_parent_array[i] = old_to_new_index[old_parent_index];
				}
				else 
				{
					subtree_parent_array[i] = i; // Make the root of the subtree point to itself
				}
			}
		}

		X::Value XGraph::SubTree(X::Value tree, long long subtree_root)
		{
			std::vector<long long> binary_tree_representation = ValueToArray(tree);

			std::vector<long long> subtree;
			std::vector<long long> new_to_old_index_map;

			extractSubtree(binary_tree_representation, subtree_root, subtree, new_to_old_index_map);
			X::List list;
			list += ArrayToValue(subtree);
			list += ArrayToValue(new_to_old_index_map);
			return list;
		}
	}
}