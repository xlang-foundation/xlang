#pragma once
#include <vector>
#include <string.h>

namespace X 
{
	template<typename T_OpInfo, typename T_OpAction>
class Lex
{
	struct KwTree
	{
		unsigned char c;
		short index;
		std::vector<KwTree> children;
		int address_base = -1;
	};
#pragma pack(push,1)
	struct nodeInfo
	{
		short index;
		unsigned char c;
		unsigned char child_cnt;
	};
#pragma pack(pop)
	void BuildFinalTree(KwTree& root, std::vector<short>& buffer)
	{
		nodeInfo n{ root.index,root.c,(unsigned char)root.children.size() };
		int curPos = (int)buffer.size();
		if (root.address_base != -1)
		{
			buffer[root.address_base] = (short)curPos;
		}
		buffer.push_back(0);
		buffer.push_back(0);
		short* p = &buffer[curPos];
		memcpy(p, &n, sizeof(nodeInfo));
		for (int i = 0; i < (int)root.children.size(); i++)
		{
			auto& child = root.children[i];
			buffer.push_back(0);
			child.address_base = (int)buffer.size() - 1;
		}
		for (int i = 0; i < (int)root.children.size(); i++)
		{
			auto& child = root.children[i];
			BuildFinalTree(child, buffer);
		}
	}
public:
	void MakeLexTree(std::vector<T_OpInfo>& opList,
		std::vector<short>& buffer,
		std::vector<T_OpAction>& opActions)
	{
		KwTree root;
		root.c = 0;
		root.index = -1;

		short lastOpIndex = 0;
		for (T_OpInfo& opInfo : opList)
		{
			short curOpIndex = -1;

			auto& k = opInfo.name;
			KwTree* node = &root;
			int len = (int)k.size();
			for (int j = 0; j < (int)k.size(); j++)
			{
				unsigned char c = k[j];
				bool bFound = false;
				for (auto& child : node->children)
				{
					if (child.c == c)
					{
						node = &child;
						if (j == (len - 1))
						{//last char, set index 
							if (node->index != -1)
							{
								curOpIndex = node->index;
								opActions[curOpIndex] = opInfo.act;
							}
							else
							{
								opActions.push_back(opInfo.act);
								node->index = lastOpIndex++;
							}
						}
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					short curOpIndex = -1;
					if (j == (len - 1))
					{
						opActions.push_back(opInfo.act);
						curOpIndex = lastOpIndex++;
					}
					node->children.push_back(
						KwTree{ c,curOpIndex }
					);
					node = &node->children[node->children.size() - 1];
				}
			}//for (int j = 0
		}
		BuildFinalTree(root, buffer);
	}
};
}