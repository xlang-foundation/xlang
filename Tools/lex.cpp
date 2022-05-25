#include "lex.h"
#include <string>
#include <vector>

//import keyword
//print(keyword.kwlist)
std::vector<std::string> _kws
{	
	//KW: --index range[0,34]
	"False", "None", "True", "and", /*4*/
	"as", "assert", "async", "await", /*4*/
	"break", "class", "continue", /*3*/
	"def", "del", "elif", "else", /*4*/
	"except", "finally", "for", /*3*/
	"from", "global", "if", "import", /*4*/
	"in", "is", "lambda", "nonlocal", /*4*/
	"not", "or", "pass", "raise", "return", /*5*/
	"try", "while", "with", "yield",/*4*/
	//Python Assignment Operators --index range[35,47]
	"=","+=","-=","*=","/=","%=","//=","**=","&=","|=","^=",">>=","<<=",
	//Python Arithmetic Operators --index range[48,54]
	"+","-","*","/","%","**","//",
	//Python Comparison Operators --index range[55,60]
	"==","!=",">","<",">=","<=",
	//Python Bitwise Operators --index range[61,66]
	"&","|","^","~","<<",">>",
	//Other  --index range[67,76]
	"(",")",":",".","[","]","{","}",",",
	//misc --index range[77,79]
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