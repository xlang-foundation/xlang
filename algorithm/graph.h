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
				APISET().AddFunc<2>("PTreeChildren", &XGraph::PTreeChildren);
				APISET().AddFunc<2>("SubTree", &XGraph::SubTree);
			END_PACKAGE
		public:
			XGraph();
			bool AddEdge(int id1, int id2, double weight);
			X::Value Prim_MST(long long root);
			X::Value PTreeChildren(X::Value tree,long long node);
			X::Value SubTree(X::Value tree, long long subtree_root);
		};
	}
}
