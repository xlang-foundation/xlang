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

#include "json.h"
#include "utility.h"
#include "port.h"
#include "dict.h"
#include "list.h"
#include <string>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <regex>
#include <stack>
#include <sstream>
#include <functional>
#include <iostream>

namespace fs = std::filesystem;

namespace X
{
	// ============================================================================
	// Helper: Parse Template to Regex
	// Syntax: "root/${year:4}/dev_${id}" -> Regex + Keys
	// ${key} -> ([a-zA-Z0-9_]+)
	// ${key:N} -> ([a-zA-Z0-9]{N})
	// ============================================================================
	// ============================================================================
	// Helper: Parse Template to Regex
	// Syntax: "root/${year:4}/dev_${id}" -> Regex + Keys
	// ${key} -> ([a-zA-Z0-9_]+)
	// ${key:N} -> ([a-zA-Z0-9]{N})
	// ============================================================================
	static void ParsePathExtractionTemplate(const std::string& tpl, std::string& regexStr, std::vector<std::string>& keys)
	{
		regexStr = "";
		keys.clear();
		size_t pos = 0;
		while (pos < tpl.size())
		{
			if (tpl[pos] == '<' && pos + 1 < tpl.size() && tpl[pos + 1] == '<')
			{
				size_t end = tpl.find(">>", pos + 2);
				if (end != std::string::npos)
				{
					std::string content = tpl.substr(pos + 2, end - (pos + 2));
					size_t colon = content.find(':');
					if (colon != std::string::npos) // <<key:N>>
					{
						std::string key = content.substr(0, colon);
						std::string lenStr = content.substr(colon + 1);
						keys.push_back(key);
						// [a-zA-Z0-9] support as requested
						regexStr += "([a-zA-Z0-9]{" + lenStr + "})"; 
					}
					else // <<key>>
					{
						keys.push_back(content);
						regexStr += "([a-zA-Z0-9_]+)";
					}
					pos = end + 2; // skip >>
					continue;
				}
			}
			
			// No auto-escaping. User provides regex or literal.
			// If they want literal '.', they must verify input or we assume they know regex.
			// Given the test case uses '.*', we must pass through.
			regexStr += tpl[pos];
			pos++;
		}
	}

	// ============================================================================
	// Logic Filter Implementation
	// ============================================================================
	enum FilterTokenType { T_ID, T_NUM, T_STR, T_AND, T_OR, T_GT, T_GTE, T_LT, T_LTE, T_EQ, T_NEQ, T_LPAREN, T_RPAREN, T_END, T_ERR };

	struct FilterToken {
		FilterTokenType type;
		std::string sVal;
		double dVal;
	};

	class FilterLogicTokenizer {
		std::string _expr;
		size_t _pos = 0;
	public:
		FilterLogicTokenizer(const std::string& expr) : _expr(expr) {}
		
		FilterToken Next() {
			while (_pos < _expr.size() && isspace(_expr[_pos])) _pos++;
			if (_pos >= _expr.size()) return { T_END, "", 0 };

			char c = _expr[_pos];
			
			// Parens
			if (c == '(') { _pos++; return { T_LPAREN, "(", 0 }; }
			if (c == ')') { _pos++; return { T_RPAREN, ")", 0 }; }

			// Ops
			if (c == '>') {
				_pos++;
				if (_pos < _expr.size() && _expr[_pos] == '=') { _pos++; return { T_GTE, ">=", 0 }; }
				return { T_GT, ">", 0 };
			}
			if (c == '<') {
				_pos++;
				if (_pos < _expr.size() && _expr[_pos] == '=') { _pos++; return { T_LTE, "<=", 0 }; }
				return { T_LT, "<", 0 };
			}
			if (c == '=') {
				_pos++;
				if (_pos < _expr.size() && _expr[_pos] == '=') { _pos++; return { T_EQ, "==", 0 }; } // ==
				return { T_EQ, "=", 0 }; // = treated as ==
			}
			if (c == '!') {
				_pos++;
				if (_pos < _expr.size() && _expr[_pos] == '=') { _pos++; return { T_NEQ, "!=", 0 }; }
				return { T_ERR, "!", 0 };
			}
			
			// String
			if (c == '\'' || c == '"') {
				char quote = c;
				_pos++;
				std::string val;
				while (_pos < _expr.size() && _expr[_pos] != quote) {
					val += _expr[_pos++];
				}
				if (_pos < _expr.size()) _pos++;
				return { T_STR, val, 0 };
			}

			// Number
			if (isdigit(c) || c == '-' || c == '.') { // simplifed, assumes proper formatting
				size_t start = _pos;
				bool isNum = false;
				if (c == '-') _pos++;
				while (_pos < _expr.size() && (isdigit(_expr[_pos]) || _expr[_pos] == '.')) {
					isNum = true;
					_pos++;
				}
				if (isNum) {
					std::string nStr = _expr.substr(start, _pos - start);
					try {
						return { T_NUM, "", std::stod(nStr) };
					} catch(...) {}
				}
				_pos = start; // backtrack if fail
			}

			// ID or Keyword (AND / OR)
			if (isalpha(c) || c == '_') {
				std::string val;
				while (_pos < _expr.size() && (isalnum(_expr[_pos]) || _expr[_pos] == '_')) {
					val += _expr[_pos++];
				}
				std::string upper = val;
				std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
				if (upper == "AND") return { T_AND, "AND", 0 };
				if (upper == "OR") return { T_OR, "OR", 0 };
				return { T_ID, val, 0 };
			}

			return { T_ERR, std::string(1, c), 0 };
		}
	};

	struct RPNComp {
		FilterToken t;
		// Precendence: ( ) = 0, OR = 1, AND = 2, CMP = 3
		int precedence() {
			switch(t.type) {
				case T_OR: return 1;
				case T_AND: return 2;
				case T_GT: case T_GTE: case T_LT: case T_LTE: case T_EQ: case T_NEQ: return 3;
				default: return 0;
			}
		}
	};

	// Convert infix logic expression to RPN
	static bool CompileFilterExpression(const std::string& expr, std::vector<FilterToken>& rpn) {
		FilterLogicTokenizer tokenizer(expr);
		std::stack<FilterToken> opStack;
		rpn.clear();
		
		while(true) {
			FilterToken t = tokenizer.Next();
			if (t.type == T_END) break;
			if (t.type == T_ERR) return false;

			if (t.type == T_NUM || t.type == T_STR || t.type == T_ID) {
				rpn.push_back(t);
			}
			else if (t.type == T_LPAREN) {
				opStack.push(t);
			}
			else if (t.type == T_RPAREN) {
				while(!opStack.empty() && opStack.top().type != T_LPAREN) {
					rpn.push_back(opStack.top());
					opStack.pop();
				}
				if(!opStack.empty()) opStack.pop(); // pop '('
			}
			else { // Ops
				int p = RPNComp{t}.precedence();
				while(!opStack.empty() && RPNComp{opStack.top()}.precedence() >= p) {
					rpn.push_back(opStack.top());
					opStack.pop();
				}
				opStack.push(t);
			}
		}
		while(!opStack.empty()) {
			if(opStack.top().type == T_LPAREN) return false; // Mismatched
			rpn.push_back(opStack.top());
			opStack.pop();
		}
		return true;
	}

	// Evaluate RPN against Metadata
	static bool EvaluateFilter(const std::vector<FilterToken>& rpn, X::Dict& meta) {
		std::stack<X::Value> stack;
		
		for(const auto& t : rpn) {
			if (t.type == T_NUM) stack.push(X::Value(t.dVal));
			else if (t.type == T_STR) stack.push(X::Value(t.sVal));
			else if (t.type == T_ID) {
				// Lookup in meta
				X::Value val = meta->Get(t.sVal); // Returns null if missing
				// Meta values are strings. Try to auto-convert to number if needed during comparison?
				// For now push as is (String or Null)
				// If meta value looks like number, we might want to convert 'on demand' in ops
				stack.push(val);
			}
			else {
				// Operators
				if (stack.size() < 2) return false;
				X::Value b = stack.top(); stack.pop();
				X::Value a = stack.top(); stack.pop();
				
				// Auto-conversion logic: if one side is number, try convert other
				double da = 0, db = 0;
				bool aIsNum = a.IsNumber();
				bool bIsNum = b.IsNumber();
				bool numericComp = aIsNum || bIsNum;
				
				// Try convert strings to numbers if comparison is numeric
				if (numericComp) {
					if (!aIsNum && a.IsString()) try { da = std::stod(a.ToString()); aIsNum = true; } catch(...) {}
					else if (aIsNum) da = (double)a;
					
					if (!bIsNum && b.IsString()) try { db = std::stod(b.ToString()); bIsNum = true; } catch(...) {}
					else if (bIsNum) db = (double)b;
				}

				bool res = false;
				switch(t.type) {
					// Logical
					case T_AND: res = (bool)a && (bool)b; break;
					case T_OR: res = (bool)a || (bool)b; break;
					
					// Comparison
					case T_GT: res = numericComp ? (da > db) : (a.ToString() > b.ToString()); break;
					case T_GTE: res = numericComp ? (da >= db) : (a.ToString() >= b.ToString()); break;
					case T_LT: res = numericComp ? (da < db) : (a.ToString() < b.ToString()); break;
					case T_LTE: res = numericComp ? (da <= db) : (a.ToString() <= b.ToString()); break;
					case T_EQ: res = numericComp ? (da == db) : (a.ToString() == b.ToString()); break;
					case T_NEQ: res = numericComp ? (da != db) : (a.ToString() != b.ToString()); break;
					default: break;
				}
				stack.push(X::Value(res));
			}
		}
		if (stack.size() != 1) return false;
		return (bool)stack.top();
	}

	// ============================================================================
	// Declarative Meta Filter Check
	// ============================================================================
	static bool CheckMetaFilter(X::Dict& meta, X::Dict& filter) {
		bool match = true;
		filter->Enum([&](X::Value& key, X::Value& val) {
			if (!match) return; // fail fast
			std::string k = key.ToString();
			X::Value metaVal = meta->Get(k);
			
			if (val.IsList()) { // Check if metaVal in list
				bool found = false;
				X::List list(val);
				for(auto item : *list) {
					if (item.ToString() == metaVal.ToString()) { found = true; break; }
				}
				if (!found) match = false;
			} else { // Direct comparison
				if (val.ToString() != metaVal.ToString()) match = false;
			}
		});
		return match;
	}

	// ============================================================================
	// LoadRaw: Implementation
	// ============================================================================
	X::Value JsonWrapper::LoadRaw(X::XRuntime* rt, X::XObj* pContext,
		X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
	{
		if (params.size() < 1) return X::Value(false);
		std::string path = params[0].ToString();
		
		// Args
		std::string pattern = "*";
		bool recursive = true;
		std::string extract_pattern = "";
		std::string filter_expr = "";
		X::Dict filter_meta;
		// X::Value filter_func; // Supported via kwParams lookup

		// Parse KW Params
		auto it = kwParams.find("pattern");
		if (it) pattern = it->val.ToString();
		
		it = kwParams.find("recursive");
		if (it) recursive = (bool)it->val;
		
		it = kwParams.find("extract_pattern");
		if (it) extract_pattern = it->val.ToString();
		
		it = kwParams.find("filter_expr");
		if (it) filter_expr = it->val.ToString();

		it = kwParams.find("filter_meta");
		if (it && it->val.IsDict()) filter_meta = it->val;

		auto itFunc = kwParams.find("filter_func"); 

// Prepare Extraction Regex
		std::regex pathRegex;
		std::vector<std::string> extractKeys;
		bool useExtraction = !extract_pattern.empty();
		if (useExtraction) {
			std::string regexStr;
			ParsePathExtractionTemplate(extract_pattern, regexStr, extractKeys);
			try {
				pathRegex = std::regex(regexStr);
			} catch(...) {
				// Invalid regex from template
				useExtraction = false;
			}
		}

		// Prepare Filter Expression
		std::vector<FilterToken> rpn;
		bool useExpr = !filter_expr.empty();
		if (useExpr) {
			if (!CompileFilterExpression(filter_expr, rpn)) {
				useExpr = false;
			}
		}

		X::List resultList;

		// Normalize Path
		if (!IsAbsPath(path)) {
			X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
			std::string curPath = pRt->M()->GetModulePath();
			path = curPath + Path_Sep_S + path;
		}

		if (!fs::exists(path)) {
			return resultList;
		}

		// Define Scan Logic
		auto processFile = [&](const fs::directory_entry& entry) {
			if (!entry.is_regular_file()) return;
			std::string fPath = entry.path().string();
			std::string fName = entry.path().filename().string();
			
			// Pattern Check (Simple glob match - suffix or wildcard)
			if (pattern != "*") {
				// Basic implementation: if pattern starts with *, check extension
				if (pattern[0] == '*') {
					std::string ext = pattern.substr(1);
					if (fName.size() < ext.size() || fName.substr(fName.size() - ext.size()) != ext) return;
				}
				// else strict match? Let's just stick to extension/glob for now or use regex if user provided non-*
			}

			X::Dict meta;
			
			// Extraction
			if (useExtraction) {
				std::string target = fPath; 
				// Standardize separators for regex match
				std::replace(target.begin(), target.end(), '\\', '/'); 
				
				std::smatch match;
				if (std::regex_search(target, match, pathRegex)) {
					for(size_t i = 0; i < extractKeys.size() && i + 1 < match.size(); i++) {
						meta->Set(extractKeys[i], X::Value(match[i+1].str()));
					}
				}
			}

			// Filters
			if (filter_meta.IsValid()) {
				if (!CheckMetaFilter(meta, filter_meta)) return;
			}
			if (useExpr) {
				if (!EvaluateFilter(rpn, meta)) return;
			}
			if (itFunc) { // filter_func
				X::Value ret;
				X::ARGS args(1); 
				args.push_back(meta);
				X::KWARGS kw;
				X::Value func = itFunc->val;
				if (func.IsObject()) {
					X::XObj* pObj = func.GetObj();
					pObj->Call(rt, pContext, args, kw, ret);
					if (!ret) return;
				}
			}

			// Load & Parse
			std::ifstream file(fPath);
			std::string line;
			int processed = 0;
			while (std::getline(file, line)) {
				if (line.empty()) continue;
				try {
					nlohmann::json j = nlohmann::json::parse(line);
					X::Value row = ConvertJsonToXValue(j);
					if (row.IsDict()) {
						X::Dict d(row);
						d->Set("_meta", meta);
						d->Set("_file", X::Value(fPath));
					}
					resultList->AddItem(row);
					processed++;
				} catch(...) {
					// Skip invalid lines
				}
			}
		};

		// Scan
		try {
			if (recursive) {
				for (const auto& entry : fs::recursive_directory_iterator(path)) processFile(entry);
			} else {
				for (const auto& entry : fs::directory_iterator(path)) processFile(entry);
			}
		} catch(std::exception& e) {
			// std::cout << "[LoadRaw] Scan Exception: " << e.what() << std::endl;
		} catch(...) {
			// std::cout << "[LoadRaw] Scan Unknown Exception" << std::endl;
		}

		retValue = resultList;
		return retValue;
	}

} // namespace X
