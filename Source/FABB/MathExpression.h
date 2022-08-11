//
//  MathExpression.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2012-01-21
//  (c) 2012-2015 yu2924
//

#pragma once
#include <string>

namespace FABB
{
	/*
	a light weight implementation of math expression evaluater
	every operators don't have any priority, and are evaluated in order. use parenthesis to grouping.
	assumes that using the character 'x' as the only named value.
	constants			: %e, %pi
	binary operators	: <=, <, >=, >, =, !=, !, &, |, +, -, *, /, %, ^
	unary operators		: !, -
	functions			: exp2, exp10, log, log2, log10, cos, sin, tan, acos, asin, atan, cosh, sinh, tanh, acosh, asinh, atanh, sqrt
	example				: "x*100", "(x^2)-(2*x)+1", "20*log10(x)", etc.
	*/
	namespace MathExpression
	{

		bool Evaluate(const char* p, double xValue, double* pv, std::string* perr = nullptr);
		bool Evaluate(const char* p, float xValue, float* pv, std::string* perr = nullptr);

	} // namespace MathExpression
} // namespace FABB
