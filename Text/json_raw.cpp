
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
#include <algorithm> // For std::sort

namespace fs = std::filesystem;

namespace X
{
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
			char c = tpl[pos];
			// Check for {key} or {key:N}
			if (c == '{')
			{
				size_t end = tpl.find('}', pos + 1);
				if (end != std::string::npos)
				{
					std::string content = tpl.substr(pos + 1, end - (pos + 1));
					size_t colon = content.find(':');
					if (colon != std::string::npos) // {key:N}
					{
						std::string key = content.substr(0, colon);
						std::string lenStr = content.substr(colon + 1);
						keys.push_back(key);
						regexStr += "(.{" + lenStr + "})"; 
					}
					else // {key}
					{
						keys.push_back(content);
						regexStr += "([^/]+)";
					}
					pos = end + 1;
					continue;
				}
			}
			// Check for ** (Recursive Wildcard) or * (Segment Wildcard)
			else if (c == '*')
			{
				if (pos + 1 < tpl.size() && tpl[pos + 1] == '*')
				{
					regexStr += ".*";
					pos += 2;
					continue;
				}
				else
				{
					regexStr += "[^/]*";
					pos++;
					continue;
				}
			}
			
			// Escape regex special chars if they are meant to be literals in the glob
			if (std::string(".^$|()[]{}+?\\").find(c) != std::string::npos)
			{
				regexStr += '\\';
			}
			regexStr += c;
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

	// Evaluate RPN against Metadata/Row
	// priority: row -> meta
	static bool EvaluateFilter(const std::vector<FilterToken>& rpn, X::Dict& meta, X::Value& row) {
		std::stack<X::Value> stack;
		X::Dict rowDict;
		if (row.IsDict()) rowDict = row;

		for(const auto& t : rpn) {
			if (t.type == T_NUM) stack.push(X::Value(t.dVal));
			else if (t.type == T_STR) stack.push(X::Value(t.sVal));
			else if (t.type == T_ID) {
				// Lookup in row first, then meta
				X::Value val;
				bool found = false;
			if (rowDict) {
					if (rowDict->Has(t.sVal)) {
						val = rowDict->Get(t.sVal);
						found = true;
					}
				}
				if (!found && meta) {
					if (meta->Has(t.sVal)) {
						val = meta->Get(t.sVal);
					}
				}
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
	// Sorting Helpers
	// ============================================================================
	struct SortSpec {
		std::string key;
		bool numeric;
		bool reverse;
	};

	static double ExtractNumber(const std::string& str) {
		// Keep [0-9], ., +, -
		std::string numStr;
		bool hasDigit = false;
		for(char c : str) {
			if (isdigit(c) || c == '.' || c == '+' || c == '-') {
				numStr += c;
				if (isdigit(c)) hasDigit = true;
			}
		}
		if (numStr.empty() || !hasDigit) return 0.0;
		try {
			return std::stod(numStr);
		} catch(...) {
			return 0.0;
		}
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
		
		// Sorting defaults
		bool global_reverse = false;
		bool global_numeric = false;
		std::vector<SortSpec> sortSpecs;
		
		long long offset = 0;
		long long limit = -1;

		// Parse KW Params
		for(auto& kw : kwParams) {
			std::string key = kw.key;
			if (key == "pattern") pattern = kw.val.ToString();
			else if (key == "recursive") recursive = (bool)kw.val;
			else if (key == "extract_pattern") extract_pattern = kw.val.ToString();
			else if (key == "filter_expr") filter_expr = kw.val.ToString();
			else if (key == "filter_meta" && kw.val.IsDict()) filter_meta = kw.val;
			else if (key == "filter_func") {
				// handled via search, or store here? 
				// The original code used itFunc iterator later. Let's store X::Value.
				// But we need to check if it's there. 
				// Let's use a flag or X::Value holder.
			}
			else if (key == "reverse") {
				global_reverse = (bool)kw.val;
			}
			else if (key == "numeric") {
				global_numeric = (bool)kw.val;
			}
			else if (key == "offset") {
				offset = kw.val.GetLongLong();
			}
			else if (key == "limit") {
				limit = kw.val.GetLongLong();
			}
		}
		
		// Handle sort and filter_func separately or in loop?
		// Sort needs precedence or complex struct? No, just parsing.
		// Re-scan for complex handling or usage?
		// filter_func was used as 'itFunc'.
		X::Value funcFilter; 
		X::Value sortVal;

		for(auto& kw : kwParams) {
			if (kw.key == std::string("filter_func")) funcFilter = kw.val;
			if (kw.key == std::string("sort")) sortVal = kw.val;
		}

		if (sortVal.IsValid()) {
			X::Value s = sortVal;
			if (s.IsList()) {
				X::List l(s);
				for(auto item : *l) {
					if (item.IsDict()) {
						X::Dict d(item);
						std::string k;
						if (d->Has("name")) k = d->Get("name").ToString();
						else if (d->Has("field")) k = d->Get("field").ToString();
						
						bool rev = global_reverse;
						if (d->Has("reverse")) rev = (bool)d->Get("reverse");

						bool num = global_numeric;
						if (d->Has("numeric")) num = (bool)d->Get("numeric");

						if (!k.empty()) sortSpecs.push_back({k, num, rev});
					} else {
						sortSpecs.push_back({item.ToString(), global_numeric, global_reverse});
					}
				}
			} else if (s.IsString()) {
				sortSpecs.push_back({s.ToString(), global_numeric, global_reverse});
			} else if (s.IsObject()) {
				// Treat generic Object as Dict-like source (e.g. PyProxy)
				std::string k;
				X::Value v = s["name"];
				if (v.IsValid()) k = v.ToString();
				else {
					v = s["field"];
					if (v.IsValid()) k = v.ToString();
				}
				
				bool rev = global_reverse;
				v = s["reverse"];
				if (v.IsValid()) rev = (bool)v;

				bool num = global_numeric;
				v = s["numeric"];
				if (v.IsValid()) num = (bool)v;

				if (!k.empty()) sortSpecs.push_back({k, num, rev});
			} else {
				sortSpecs.push_back({s.ToString(), global_numeric, global_reverse});
			}
		}

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
				useExtraction = false;
			}
		}

		// Prepare Filter Expression
		std::vector<FilterToken> rpn;
		bool useExpr = !filter_expr.empty();
		bool fileLevelFilter = false;
		if (useExpr) {
			if (CompileFilterExpression(filter_expr, rpn)) {
				// Check if all IDs in RPN are in extractKeys
				// If so, we can filter at file level
				bool allKnown = true;
				for(const auto& t : rpn) {
					if (t.type == T_ID) {
						bool known = false;
						for(const auto& k : extractKeys) {
							if (k == t.sVal) { known = true; break; }
						}
						if (!known) { allKnown = false; break; }
					}
				}
				fileLevelFilter = allKnown;
			} else {
				useExpr = false;
			}
		}

		std::vector<X::Value> items; // Collect here for sort

		// Normalize Path separator
		std::string normPath = path;
		std::replace(normPath.begin(), normPath.end(), '\\', '/');
		
		if (!IsAbsPath(normPath)) {
			X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
			std::string curPath = pRt->M()->GetModulePath();
			std::replace(curPath.begin(), curPath.end(), '\\', '/');
			normPath = curPath + "/" + normPath;
		}

		if (!fs::exists(normPath)) {
			X::List emptyList;
			retValue = emptyList;
			return emptyList;
		}

		// Define Scan Logic
		auto processFile = [&](const fs::directory_entry& entry) {
			if (!entry.is_regular_file()) return;
			std::string fPath = entry.path().string();
			std::string fName = entry.path().filename().string();
			
			// Normalize for matching
			std::string target = fPath;
			std::replace(target.begin(), target.end(), '\\', '/');
			
			if (pattern != "*") {
				if (pattern.size() > 0 && pattern[0] == '*') {
					std::string ext = pattern.substr(1);
					if (fName.size() < ext.size() || fName.substr(fName.size() - ext.size()) != ext) return;
				}
			}

			X::Dict meta;
			
			// Extraction
			if (useExtraction) {
				std::smatch match;
				if (std::regex_search(target, match, pathRegex)) {
					for(size_t i = 0; i < extractKeys.size() && i + 1 < match.size(); i++) {
						meta->Set(extractKeys[i], X::Value(match[i+1].str()));
					}
				}
			}

			// File Level Filters
			if (filter_meta) {
				if (!CheckMetaFilter(meta, filter_meta)) return;
			}
			
			// Optimization: Apply filter expr here if it ONLY depends on meta
			X::Value dummy;
			if (useExpr && fileLevelFilter) {
				if (!EvaluateFilter(rpn, meta, dummy)) return;
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

					// Row Level Filter: Check filter expr if it depends on row content
					if (useExpr && !fileLevelFilter) {
						if (!EvaluateFilter(rpn, meta, row)) continue;
					}

					if (row.IsDict()) {
						X::Dict d(row);
						d->Set("_meta", meta);
						d->Set("_file", X::Value(fPath));
					}
					
					// Filter Func check (post-parse)
					if (funcFilter.IsValid()) { 
						X::Value ret;
						X::ARGS args(1); 
						args.push_back(row); // Pass full row
						X::KWARGS kw;
						X::Value func = funcFilter;
						if (func.IsObject()) {
							X::XObj* pObj = func.GetObj();
							pObj->Call(rt, pContext, args, kw, ret);
							if (!ret) continue;
						}
					}

					items.push_back(row);
					processed++;
				} catch(...) {
					// Skip invalid lines
				}
			}
		};

		// Scan
		try {
			if (recursive) {
				for (const auto& entry : fs::recursive_directory_iterator(normPath)) processFile(entry);
			} else {
				for (const auto& entry : fs::directory_iterator(normPath)) processFile(entry);
			}
		} catch(...) {
		}

		// Sort
		if (!sortSpecs.empty()) {
			std::sort(items.begin(), items.end(), [&](const X::Value& a, const X::Value& b) -> bool {
				X::Dict da(a);
				X::Dict db(b);
				
				for(const auto& spec : sortSpecs) {
					X::Value va = da->Get(spec.key);
					X::Value vb = db->Get(spec.key);
					
					// Compare
					int diff = 0;
					
					if (spec.numeric) {
						double na = ExtractNumber(va.ToString());
						double nb = ExtractNumber(vb.ToString());
						if (na < nb) diff = -1;
						else if (na > nb) diff = 1;
					} else {
						// Lexical with X::Value comparison
                        // operator== and operator< are available
                        if (va == vb) diff = 0;
						else if (va < vb) diff = -1;
						else diff = 1;
					}

					if (diff != 0) {
						if (spec.reverse) return diff > 0;
						else return diff < 0;
					}
				}
				return false; // Equal
			});
		}

		// Slice
		if (offset > 0) {
			if (offset >= (long long)items.size()) {
				items.clear();
			} else {
				items.erase(items.begin(), items.begin() + offset);
			}
		}
		if (limit >= 0 && limit < (long long)items.size()) {
			items.resize(limit); // reduce size
		}

		X::List resultList;
		for(auto& i : items) resultList->AddItem(i);
		retValue = resultList;
		return retValue;
	}

} // namespace X
