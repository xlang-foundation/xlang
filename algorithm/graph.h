/*
Copyright (C) 2024 The XLang Foundation
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

#pragma once
#include "xpackage.h"
#include "xlang.h"


namespace X
{
	namespace Algorithm
	{
		class XGraph
		{
			void* mGraph = nullptr;
		public:
			BEGIN_PACKAGE(XGraph)
				APISET().AddFunc<3>("AddEdge", &XGraph::AddEdge);
				APISET().AddFunc<1>("Prim_MST", &XGraph::Prim_MST);
				APISET().AddFunc<2>("Dijkstra_Shortest_Paths", &XGraph::Dijkstra_Shortest_Paths);
				APISET().AddFunc<2>("PTreeChildren", &XGraph::PTreeChildren);
				APISET().AddFunc<2>("SubTree", &XGraph::SubTree);
			END_PACKAGE
		public:
			XGraph();
			bool AddEdge(int id1, int id2, double weight);
			X::Value Prim_MST(long long root);
			X::Value Dijkstra_Shortest_Paths(long long start,long long end);
			X::Value PTreeChildren(X::Value tree,long long node);
			X::Value SubTree(X::Value tree, long long subtree_root);
		};
	}
}
