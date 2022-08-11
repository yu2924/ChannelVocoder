//
//  ParamConvert.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2012-01-20
//  (c) 2012-2017 yu2924
//

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace FABB
{

	/*
	initializer string contains multiple sections
	each section are separated by tab '\t' characters
	<key>\t<displaytext>\t<controlrange>\t<valueconverter>\t<stringconverter>

	<key>
		unique string identifier
		e.g. "param1"

	<displaytext>
		human readable display text
		each subsection are separated by semicolon ';' characters
		<label>;<unit>
			<label>: parameter name
			<unit>: parameter unit (optional)
		e.g. "Frequency;Hz"

	<controlrange>
		defines range of the parameter value, the defaule value, and optionally the native type
		each subsection are separated by semicolon ';' characters
		<range>;<default>[;<type>]
			<range>: two real numbers separated by the characters '~'
			<default>: a real number, (optional) with prefix 'N', it means the value is specified in native value space
			<type>: type specifier for native value, "float" or "int"
		e.g. "0~1;0.5"
		e.g. "0~1;N10" // in native value
		e.g. "0~1;0.5;int" // type of native value is integer

	<valueconverter>
		defines relationship of ('control' <=> 'native') values for the signal prosessing purpose
		multiple regions can be included
		each region are separated by semicolon ';' characters
		<region1>;<region2>;...
			each region contains {curve, range} separated by exclamation '!' characters
			available curve types are: {pt, enum, lin, exp}
			<pt>!<controlvalue>!<nativevalue>
			<enum>!<controlvalue>!<nativevalue1,nativevalue2,...>
			<lin>!<controlrange>!<nativerange>
			<exp>!<controlrange>!<nativerange>
				<controlvalue>: a real number, for example "0"
				<nativevalue>: a real number, for example "0"
				<controlrange>: two real numbers separated by the characters '~', e.g. "0~1"
				<nativerange>: two real numbers separated by the characters '~', e.g. "0~1"
		e.g. "enum!0~1!1,2,3"
		e.g. "pt!0!0;lin!0~1!0~1"

	<stringconverter>
		defines relationship of ('control' <=> 'native') values for the human readable displaying purpose
		multiple regions can be included
		each region are separated by semicolon ';' characters
		<region1>;<region2>;...
			each region contains {curve, range, format} separated by exclamation '!' characters
			available curve types are: {pt, enum, lin, exp}
			<pt>!<controlvalue>!<displaytext>
			<enum>!<controlvalue>!<displaytext1,displaytext2,...>
			<lin>!<controlrange>!<printformat,param1,param2,...>!<scanformat,param>
			<exp>!<controlrange>!<printformat,param1,param2,...>!<scanformat,param>
				<controlvalue>: a real number, for example "0"
				<displaytext>: a human readable text, e.g. "one", "two", "three", etc.
				<controlrange>: two real numbers separated by the characters '~', e.g. "0~1"
				<printformat>: 'printf' style string and following arguments separated by comma ',' characters
				<scanformat>: 'scanf' style string and following an argument separated by comma ',' characters
					supports only float type specifiers: {A, a, E, e, F, f, G, g, K, k}
					the {K, k} are custom type specifier using unit prefixes, and the presision specify number of effective digits
					supported unit prefixes are: {"f", "p", "n", "u", "m", "k", "M", "G", "T", "P"}
					for example: print("%.4k", 0.1234) displays "123.4m", print("%.4k", 1234) displays "1.234k", etc.
					each argument are evaluated as a math expression (cf. MathExpression.h)
					for example: "x*100", "(1-x)*100", etc.
		e.g. "enum!0~1!one,two,three"
		e.g. "pt!0!off;lin!0~1!%f,x!%f,x"

	examples:

	"Vol" "\t" "Volume;dB" "\t" "0~1;1" "\t" "pt!0!0; exp!0~1!0.001~1" "\t" "pt!0!Off; lin!0~1!-60~0!%.0f,x!%f,x"
		Volume, control exponentially, display linearly in dB with particular point 'Off'
		will display {"Off", "-45", "-30", "-15", 0}

	"Pan" "\t" "Pan" "\t" "0~1;0.5" "\t" "lin!0~1!-1~1" "\t" "pt!0!L; pt!0.5!C; pt!1!R; lin!0~1!-100~100!%.0f,x!%f,x"
		Panpot, control linearly, display linearly in percent with particular points {L, C, R}
		will display {"L", "-50", "C", "50", "R"}

	"Bal" "\t" "Balance;%" "\t" "0~1;0.5" "\t" "lin!0~1!-1~1" "\t" "lin!0~1!0~1!%03.0f:%03.0f,x*100,(1-x)*100!%f,x/100"
		L-R Balance, control linearly, display linearly as L-R ratio
		will display {"100:000", "075:025", "050:050", "025:075", "000:100"}

	"WF" "\t" "Waveform" "\t" "0~1;0" "\t" "enum!0~1!1,2,3" "\t" "enum!0~1!saw,tri,rect"
		Waveform selection list, maps as {1:saw, 2:tri, 3:rect}
		will display {"saw", "tri", "rect"}

	"Freq" "\t" "Frequency;Hz" "\t" "0~1;0" "\t" "exp!0~1!1~1000000" "\t" "exp!0~1!1~1000000!%.4k,x!%k,x"
		Frequency, control exponentially, display exponentially in Hz using unit prefix {k, M}
		will display {"1.000", "31.62", "1.000k", "31.62k", "1.000M"}
	*/

	class IValueConverter;
	class IStringConverter;

	class ParamConverter
	{
	protected:
		std::string mKey, mName, mUnit;
		float mVcMin, mVcMax, mVcDef;
		bool mIsEnum, mIsInteger;
		std::vector<std::unique_ptr<IValueConverter> > mValConv;
		std::vector<std::unique_ptr<IStringConverter> > mStrConv;
	public:
		ParamConverter(const std::string& s);
		~ParamConverter();
		void Load(const std::string& s);
		void Clear();
		const char* Key() const;
		const char* Name() const;
		const char* Unit() const;
		float ControlMin() const;
		float ControlMax() const;
		float ControlDef() const;
		float LimitControlValue(float vc) const;
		float NativeMin() const;
		float NativeMax() const;
		float NativeDef() const;
		float LimitNativeValue(float vn) const;
		float ControlToNative(float vc) const;
		float NativeToControl(float vn) const;
		// IsInteger: include both 'continuous integers' and 'enum values'
		bool IsInteger() const;
		int ControlToNativeInt(float vc) const;
		float NativeIntToControl(int vn) const;
		bool IsEnum() const;
		int GetEnumCount() const;
		std::vector<std::string> GetEnumStrings() const;
		int ControlToEnumIndex(float vc) const;
		float EnumIndexToControl(int ii) const;
		std::string Format(float vc) const;
		float Parse(const std::string& s) const;
	};

	class ParamConverterTable
	{
	protected:
		std::vector<std::shared_ptr<ParamConverter> > mTable;
		std::map<std::string, ParamConverter*> mMap;
		void rebuildmap();
	public:
		ParamConverterTable();
		// uses array of strings as initializers for multiple parameters
		ParamConverterTable(const char* const* ps, size_t cs);
		// uses '\r' or '\n' separated string as initializers for multiple parameters
		ParamConverterTable(const char* s);
		ParamConverterTable(const ParamConverterTable& r);
		ParamConverterTable& operator=(const ParamConverterTable& r);
		ParamConverterTable(ParamConverterTable&& r);
		ParamConverterTable& operator=(ParamConverterTable&& r);
		~ParamConverterTable();
		// uses array of strings as initializers for multiple parameters
		void Load(const char* const* ps, size_t cs);
		// uses '\r' or '\n' separated string as initializers for multiple parameters
		void Load(const char* s);
		void Clear();
		void replaceWith(size_t i, std::shared_ptr<ParamConverter> cvt);
		size_t Count() const;
		const ParamConverter* At(size_t i) const;
		const ParamConverter* operator[](size_t i) const;
		const ParamConverter* At(const char* k) const;
		const ParamConverter* operator[](const char* k) const;
		ParamConverter* At(size_t i);
		ParamConverter* operator[](size_t i);
		ParamConverter* At(const char* k);
		ParamConverter* operator[](const char* k);
	};

} // namespace FABB
