//
//  MathExpression.cpp
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2012-01-21
//  (c) 2012-2015 yu2924
//

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <list>
#include <string>
#include "MathExpression.h"

namespace FABB
{
	namespace MathExpression
	{

		static inline int CHARS(int ch) { return ch; }
		static inline int CHARS(int ch1, int ch2) { return (ch1 << 8) | ch2; }
		static inline const char* skipblank(const char* p) { while(isspace(*p)) p ++; return p; }
		static inline const char* skipalnum(const char* p) { while(isalnum(*p)) p ++; return p; }

		static const char* findparenthesisrange(const char* p)
		{
			// assumes *p=='('
			int nestlevel = 0;
			const char* pf = p + 1; while(*pf)
			{
				if(*pf == CHARS('(')) { nestlevel ++; pf ++; continue; }
				else if(*pf == CHARS(')')) { if(nestlevel == 0) return pf + 1; nestlevel --; pf ++; continue; } // found
				else { pf ++; continue; }
			}
			return nullptr;
		}

		static std::string strlower(const std::string& s)
		{
			std::string sr = s;
			std::for_each(sr.begin(), sr.end(), [](char& c) { c = (char)tolower(c); });
			return sr;
		}

		struct Token
		{
			union
			{
				double value;
				int opcode;
			};
			enum { type_value, type_opcode } type;
			Token(double v) { value = v; type = type_value; }
			Token(int c) { opcode = c; type = type_opcode; }
			bool operator==(double v) const { return (type == type_value) && (value == v); }
			bool operator==(int c) const { return  (type == type_opcode) && (opcode == c); }
			bool isvalue() const { return type == type_value; }
			bool isopcode() const { return type == type_opcode; }
		};

		using TokenList = std::list<Token>;

		static bool getnextopcode(const TokenList& tokens, TokenList::const_iterator& i, int* pv)
		{
			if(i == tokens.end() || !i->isopcode()) return false;
			*pv = i->opcode; i ++; return true;
		}

		static bool getnextvalue(const TokenList& tokens, TokenList::const_iterator& i, double* pv)
		{
			if(i == tokens.end()) return false;
			if(i->isopcode())
			{
				// process unary operators
				int op = i->opcode; i ++;
				if(i == tokens.end() || !i->isvalue()) return false;
				if(op == CHARS('!')) { *pv = (i->value == 0) ? 1 : 0; i ++; return true; }
				if(op == CHARS('-')) { *pv = -i->value; i ++; return true; }
				else return false;
			}
			else { *pv = i->value; i ++; return true; }
		}

		bool Evaluate(const char* p, double xValue, double* pv, std::string* perr)
		{
			if(!p || !*p) { if(perr) *perr = "invalid parameters"; return false; }
			// parse tokens
			TokenList tokens;
			while(*p)
			{
				p = skipblank(p);
				if(!*p) break;
				else if(*p == '(')
				{
					const char* ppare = findparenthesisrange(p);
					if(!ppare) { if(perr) *perr = "parenthesis missmatch"; return false; }
					double v = 0; if(!Evaluate(std::string(p + 1, ppare - p - 2).c_str(), xValue, &v, perr)) return false;
					tokens.push_back(Token(v));
					p = ppare;
				}
				else if(strncmp(p, "%e", 2) == 0) { tokens.push_back(Token(2.71828182845904523536));	p += 2; }
				else if(strncmp(p, "%pi", 3) == 0) { tokens.push_back(Token(3.14159265358979323846));	p += 3; }
				else if(strncmp(p, "<=", 2) == 0) { tokens.push_back(Token(CHARS('<', '=')));			p += 2; }
				else if(strncmp(p, "<", 1) == 0) { tokens.push_back(Token(CHARS('<')));				p += 1; }
				else if(strncmp(p, ">=", 2) == 0) { tokens.push_back(Token(CHARS('>', '=')));			p += 2; }
				else if(strncmp(p, ">", 1) == 0) { tokens.push_back(Token(CHARS('>')));				p += 1; }
				else if(strncmp(p, "=", 1) == 0) { tokens.push_back(Token(CHARS('=')));				p += 1; }
				else if(strncmp(p, "!=", 2) == 0) { tokens.push_back(Token(CHARS('!', '=')));			p += 2; }
				else if(*p == '!') { tokens.push_back(Token(CHARS('!'))); p ++; }
				else if(*p == '&') { tokens.push_back(Token(CHARS('&'))); p ++; }
				else if(*p == '|') { tokens.push_back(Token(CHARS('|'))); p ++; }
				else if(*p == '+') { tokens.push_back(Token(CHARS('+'))); p ++; }
				else if(*p == '-') { tokens.push_back(Token(CHARS('-'))); p ++; }
				else if(*p == '*') { tokens.push_back(Token(CHARS('*'))); p ++; }
				else if(*p == '/') { tokens.push_back(Token(CHARS('/'))); p ++; }
				else if(*p == '%') { tokens.push_back(Token(CHARS('%'))); p ++; }
				else if(*p == '^') { tokens.push_back(Token(CHARS('^'))); p ++; }
				else if(*p == 'x') { tokens.push_back(Token(xValue)); p ++; }
				else if(isalpha(*p)) // functions
				{
					const char* ptage = skipalnum(p);
					if(!*ptage) { if(perr) *perr = "invalid token"; return false; }
					const char* ppar = skipblank(ptage);
					if(*ppar != '(') { if(perr) *perr = "invalid sequence"; return false; }
					const char* ppare = findparenthesisrange(ppar);
					if(!ppare) { if(perr) *perr = "parenthesis missmatch"; return false; }
					double v = 0; if(!Evaluate(std::string(ppar + 1, ppare - ppar - 2).c_str(), xValue, &v, perr)) return false;
					std::string tag = strlower(std::string(p, ptage - p));
					if(tag == "exp") v = std::exp(v);
					else if(tag == "exp2") v = std::pow(2, v);
					else if(tag == "exp10") v = std::pow(10, v);
					else if(tag == "log") v = std::log(v);
					else if(tag == "log2") v = std::log2(v);
					else if(tag == "log10") v = std::log10(v);
					else if(tag == "cos") v = std::cos(v);
					else if(tag == "sin") v = std::sin(v);
					else if(tag == "tan") v = std::tan(v);
					else if(tag == "acos") v = std::acos(v);
					else if(tag == "asin") v = std::asin(v);
					else if(tag == "atan") v = std::atan(v);
					else if(tag == "cosh") v = std::cosh(v);
					else if(tag == "sinh") v = std::sinh(v);
					else if(tag == "tanh") v = std::tanh(v);
					else if(tag == "acosh") v = std::acosh(v);
					else if(tag == "asinh") v = std::asinh(v);
					else if(tag == "atanh") v = std::atanh(v);
					else if(tag == "sqrt") v = std::sqrt(v);
					else { if(perr) *perr = "invalid token"; return false; }
					tokens.push_back(Token(v));
					p = ppare;
				}
				else // real values
				{
					char* pn = nullptr; double v = std::strtod(p, &pn);
					if(pn == p) { if(perr) *perr = "invalid token"; return false; }
					tokens.push_back(Token(v));
					p = pn;
				}
			}
			if(tokens.empty()) return false;
			// process tokens
			// first value
			TokenList::const_iterator i = tokens.begin();
			double vr = 0; if(!getnextvalue(tokens, i, &vr)) { if(perr) *perr = "invalid sequence"; return false; }
			// next binary operator and value pair
			while(i != tokens.end())
			{
				// process binary operators
				int op = 0; if(!getnextopcode(tokens, i, &op)) { if(perr) *perr = "invalid sequence"; return false; }
				double vn = 0; if(!getnextvalue(tokens, i, &vn)) { if(perr) *perr = "invalid sequence"; return false; }
				if(op == CHARS('<', '=')) vr = (vr <= vn) ? 1 : 0;
				else if(op == CHARS('<')) vr = (vr < vn) ? 1 : 0;
				else if(op == CHARS('>', '=')) vr = (vr >= vn) ? 1 : 0;
				else if(op == CHARS('>')) vr = (vr > vn) ? 1 : 0;
				else if(op == CHARS('=')) vr = (vr == vn) ? 1 : 0;
				else if(op == CHARS('!', '=')) vr = (vr != vn) ? 1 : 0;
				else if(op == CHARS('&')) vr = (vr && vn) ? 1 : 0;
				else if(op == CHARS('|')) vr = (vr || vn) ? 1 : 0;
				else if(op == CHARS('+')) vr += vn;
				else if(op == CHARS('-')) vr -= vn;
				else if(op == CHARS('*')) vr *= vn;
				else if(op == CHARS('/')) vr /= vn;
				else if(op == CHARS('%')) vr = std::fmod(vr, vn);
				else if(op == CHARS('^')) vr = std::pow(vr, vn);
				else { if(perr) *perr = "invalid sequence"; return false; }
			}
			*pv = vr;
			return true;
		}

		bool Evaluate(const char* p, float xValue, float* pv, std::string* perr)
		{
			double vo = 0; if(!Evaluate(p, xValue, &vo, perr)) return false;
			*pv = (float)vo;
			return true;
		}

	} // namespace MathExpression
} // namespace FABB
