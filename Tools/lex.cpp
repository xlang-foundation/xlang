#include "lex.h"
#include <string>
#include <vector>

//import keyword
//print(keyword.kwlist)
std::vector<std::string> _kws
{
	"False", "None", "True", "and", 
	"as", "assert", "async", "await", 
	"break", "class", "continue", 
	"def", "del", "elif", "else", 
	"except", "finally", "for", 
	"from", "global", "if", "import", 
	"in", "is", "lambda", "nonlocal", 
	"not", "or", "pass", "raise", "return", 
	"try", "while", "with", "yield",
	//Python Arithmetic Operators
	"+","-","*","/","%","**","//",
	//Python Assignment Operators
	"=","+=","-=","*=","/=","%=","//=","**=","&=","|=","^=",">>=","<<=",
	//Python Comparison Operators
	"==","!=",">","<",">=","<=",
	//Python Bitwise Operators
	"&","|","^","~","<<",">>",
	//Other
	"(",")",":",".","[","]","{","}",
	//misc
	"\t","\n","\r"
};
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
void BuildFinalTree(KwTree& root,std::vector<short>& buffer)
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
void MakeLexTree(std::vector<short>& buffer)
{
	KwTree root;
	root.c = 0;
	root.index = -1;

	for (int i = 0; i < (int)_kws.size(); i++)
	{
		auto k = _kws[i];
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
						node->index = i;
					}
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				node->children.push_back(
					KwTree{ c,(j == (len - 1)) ? (short)i : (short) - 1}
				);
				node = &node->children[node->children.size()-1];
			}
		}
	}
	BuildFinalTree(root, buffer);
}