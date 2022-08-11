//
//  ParamConvert.cpp
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2012-01-20
//  (c) 2012-2017 yu2924
//

#if defined _MSC_VER && !defined _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>
#include "CurveMapping.h"
#include "MathExpression.h"
#include "ParamConvert.h"

#if defined _MSC_VER
#pragma warning(disable:4127)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#define countof(a) (sizeof(a)/sizeof(a[0]))

namespace FABB
{

	//==============================================================================
	// utilities

	namespace ParamUtil
	{
		static std::vector<std::string> Tokenize(const std::string& s, const std::string& del)
		{
			std::vector<std::string> vs;
			size_t lastPos = s.find_first_not_of(del, 0);
			size_t pos = s.find_first_of(del, lastPos);
			while((std::string::npos != pos) || (std::string::npos != lastPos))
			{
				vs.push_back(s.substr(lastPos, pos - lastPos));
				lastPos = s.find_first_not_of(del, pos);
				pos = s.find_first_of(del, lastPos);
			}
			return vs;
		}

		static std::vector<std::string> Tokenize(const std::string& s, char del)
		{
			std::vector<std::string> vs;
			const char* p = s.c_str();
			const char* pe = p + s.size();
			while(p <= pe)
			{
				const char* pn = strchr(p, del); if(!pn) pn = pe;
				vs.push_back(std::string(p, pn - p));
				p = pn + 1;
			}
			return vs; // std::move is not required beacuse of 'return value optimization (RVO)'
		}

		static std::string Trim(const std::string& s, const std::string& del = "\t\r\n ")
		{
			if(s.length() == 0) return s;
			size_t i0 = s.find_first_not_of(del);
			if(i0 == std::string::npos) return std::string();
			size_t i1 = s.find_last_not_of(del);
			return std::string(s, i0, i1 - i0 + 1); // std::move is not required beacuse of 'return value optimization (RVO)'
		}

		static int RankMatch(const std::string& sa, const std::string& sb)
		{
			if(strcmp(sa.c_str(), sb.c_str()) == 0) return 4;
			if(strcasecmp(sa.c_str(), sb.c_str()) == 0) return 3;
			size_t l = (sa.size() < sb.size()) ? sa.size() : sb.size();
			if(strncmp(sa.c_str(), sb.c_str(), l) == 0) return 2;
			if(strncasecmp(sa.c_str(), sb.c_str(), l) == 0) return 1;
			return 0;
		}

		static size_t FindMostMatch(const std::vector<std::string>& vs, const std::string& s)
		{
			std::vector<int> vr(vs.size(), 0);
			for(size_t c = vs.size(), i = 0; i < c; i ++) vr[i] = RankMatch(vs[i], s);
			return std::distance(vr.begin(), std::max_element(vr.begin(), vr.end()));
		}

		template<typename T> static bool ParseValue(const std::string& s, T* pv)
		{
			return MathExpression::Evaluate(s.c_str(), 0, pv);
		}

		template<typename T> static bool ParseRange(const std::string& s, T* pl, T* ph)
		{
			// e.g. s="0~1"
			std::vector<std::string> vs = Tokenize(s, '~');
			if(vs.size() < 2) return false;
			return ParseValue(vs[0], pl) && ParseValue(vs[1], ph);
		}

		template<class T> static T Limit(T vl, T vh, T v)
		{
			return (v <= vh) ? ((vl <= v) ? v : vl) : vh;
		}

		template<class T> static T LimitAbs(T va, T vb, T v)
		{
			T vl, vh;
			if(va <= vb) { vl = va; vh = vb; }
			else		 { vl = vb; vh = va; }
			return (v <= vh) ? ((vl <= v) ? v : vl) : vh;
		}

	} // namespace ParamUtil

	class PrintFormat
	{
	public:
		// support special '%k' format
		struct Part
		{
			std::string fmt, arg;
			char precision; // used in the case pfxunit ('%k') as the number of effective digits
			bool pfxunit;
			Part(const std::string& f, const std::string& a, char pr, bool pu) : fmt(f), arg(a), precision(pr), pfxunit(pu) {}
		};
		std::vector<Part> mParts;
		void Setup(const std::string& f, const std::vector<std::string>& a)
		{
			static const std::regex rx("%([ #+\\-0-9]*)\\.?([0-9]*)([AEFGKaefgk])");
			mParts.clear();
			std::string infmt(f);
			std::smatch m;
			size_t iarg = 0;
			while(std::regex_search(infmt, m, rx))
			{
				mParts.push_back(Part(m.prefix().str(), "", 0, false));
				assert(m.size() == 4);
				std::string part("%");
				std::string opt1 = m[1].str();
				std::string opt2 = m[2].str();
				std::string ts = m[3].str();
				bool pfx = strcasecmp(ts.c_str(), "k") == 0;
				char prec = (char)std::stoi(opt2);
				if(pfx && (prec <= 0)) prec = 4; // 4 digits by default
				// options
				part += opt1;
				if(pfx) opt2 = "X"; // '%.Xf', precision placeholder
				part += "." + opt2;
				// type specifier
				if(pfx) ts = "f";
				part += ts;
				if(pfx) part += "%s";
				mParts.push_back(Part(part, a[iarg ++], prec, pfx));
				// next
				infmt = m.suffix().str();
			}
			// last suffix
			mParts.push_back(Part(infmt, "", 0, false));
		}
		std::string Print(float v) const
		{
			std::string sr;
			for(auto part : mParts)
			{
				if(part.arg.empty()) // literal
				{
					sr += part.fmt;
				}
				else
				{
					double vp = 0; MathExpression::Evaluate(part.arg.c_str(), (double)v, &vp);
					std::string fmt = part.fmt;
					const char* pfx = "";
					if(part.pfxunit)
					{
						// unit prefix
						double avp = std::abs(vp);
						if     (1e15  <= avp) { vp *= 1e-15; pfx = "P"; }
						else if(1e12  <= avp) { vp *= 1e-12; pfx = "T"; }
						else if(1e9   <= avp) { vp *= 1e-9; pfx = "G"; }
						else if(1e6   <= avp) { vp *= 1e-6; pfx = "M"; }
						else if(1e3   <= avp) { vp *= 1e-3; pfx = "k"; }
						else if(1e0   <= avp) {}
						else if(1e-3  <= avp) { vp *= 1e3; pfx = "m"; }
						else if(1e-6  <= avp) { vp *= 1e6; pfx = "u"; }
						else if(1e-9  <= avp) { vp *= 1e9; pfx = "n"; }
						else if(1e-12 <= avp) { vp *= 1e12; pfx = "p"; }
						else { vp *= 1e15; pfx = "f"; }
						// determine number of digits to appear after the decimal point
						avp = std::abs(vp);
						int dig = 0;
						do
						{
							// NOTE: take account of rounding up the fraction under the minimum digit
							dig = std::max(0, std::min(9, (int)part.precision - 3));
							if((100 - pow(10, -dig) * 0.05) <= avp) break;
							dig = std::max(0, std::min(9, (int)part.precision - 2));
							if((10 - pow(10, -dig) * 0.05) <= avp) break;
							dig = std::max(0, std::min(9, (int)part.precision - 1));
							if((1 - pow(10, -dig) * 0.05) <= avp) break;
							dig = std::max(0, std::min(9, (int)part.precision));
							break;
						} while(true);
						std::replace(fmt.begin(), fmt.end(), 'X', (char)('0' + dig));
					}
					char s[128];
#if defined _MSC_VER
					_snprintf_s(s, countof(s), fmt.c_str(), vp, pfx);
#else
					std::snprintf(s, countof(s), fmt.c_str(), vp, pfx);
#endif
					sr += s;
				}
			}
			return sr;
		}
	};

	class ScanFormat
	{
	public:
		std::string mFmt, mArg;
		bool mPfxUnit;
		void Setup(const std::string& f, const std::string& a)
		{
			static const std::regex rx("%([*0-9]*)([AEFGKaefgk])");
			std::smatch m;
			if(!std::regex_search(f, m, rx))
			{
				assert(false);
				mFmt = f;
				mArg = "";
				mPfxUnit = false;
			}
			else
			{
				assert(m.size() == 3);
				mFmt = m.prefix();
				mFmt += "%";
				// options
				mFmt += m[1].str();
				// type specifier
				std::string ts = m[2].str();
				bool pfx = strcasecmp(ts.c_str(), "k") == 0;
				if(pfx) ts = "f";
				mFmt += "l" + ts; // double expected
				if(pfx) mFmt += "%c";
				mFmt += m.suffix().str();
				mArg = a;
				mPfxUnit = pfx;
			}
		}
		bool Scan(const std::string& s, float* pv) const
		{
			double v = 0; char pfx = 0;
#if defined _MSC_VER
			int n = sscanf_s(s.c_str(), mFmt.c_str(), &v, &pfx, sizeof(pfx));
#else
			int n = std::sscanf(s.c_str(), mFmt.c_str(), &v, &pfx);
#endif
			if(n <= 0) return false;
			if(mPfxUnit && (n == 2))
			{
				if     (pfx          == 'P') v *= 1e15;
				else if(toupper(pfx) == 'T') v *= 1e12;
				else if(toupper(pfx) == 'G') v *= 1e9;
				else if(pfx          == 'M') v *= 1e6;
				else if(tolower(pfx) == 'k') v *= 1e3;
				else if(pfx          == 'm') v *= 1e-3;
				else if(tolower(pfx) == 'u') v *= 1e-6;
				else if(tolower(pfx) == 'n') v *= 1e-9;
				else if(pfx          == 'p') v *= 1e-12;
				else if(tolower(pfx) == 'f') v *= 1e-15;
			}
			return MathExpression::Evaluate(mArg.c_str(), (float)v, pv);
		}
	};

	//==============================================================================
	// base interfaces

	class IValueConverter
	{
	public:
		virtual ~IValueConverter() {}
		virtual bool ControlToNative(float vc, float* pvn) const = 0;
		virtual bool NativeToControl(float vn, float* pvc) const = 0;
	};

	class IStringConverter
	{
	public:
		virtual ~IStringConverter() {}
		virtual bool IsEnum() const = 0;
		virtual int GetEnumCount() const = 0;
		virtual std::vector<std::string> GetEnumStrings() const = 0;
		virtual bool ControlToEnumIndex(float vc, int* pi) const = 0;
		virtual bool EnumIndexToControl(int i, float* pvc) const = 0;
		virtual bool Format(float vc, std::string* ps) const = 0;
		virtual bool Parse(const std::string& s, float* pvc) const = 0;
	};

	//==============================================================================
	// value converters

	class PtValueConverter : public IValueConverter
	{
	public:
		float mVc, mVn;
		bool mPermissive;
		PtValueConverter(const std::string& ini, bool permissive)
		{
			// e.g. ini="pt!0!0";
			std::vector<std::string> vs = ParamUtil::Tokenize(ini, '!');
			assert(vs.size() == 3);
			ParamUtil::ParseValue(vs[1], &mVc);
			ParamUtil::ParseValue(vs[2], &mVn);
			mPermissive = permissive;
		}
		virtual bool ControlToNative(float vc, float* pvn) const
		{
			if(mPermissive) vc = mVc;
			if(vc == mVc) { *pvn = mVn; return true; }
			return false;
		}
		virtual bool NativeToControl(float vn, float* pvc) const
		{
			if(mPermissive) vn = mVn;
			if(vn == mVn) { *pvc = mVc; return true; }
			return false;
		}
	};

	class EnumValueConverter : public IValueConverter
	{
	public:
		float mVcl, mVch;
		std::vector<int> mIndices;
		CurveMapLinearF mMap;
		bool mPermissive;
		EnumValueConverter(const std::string& ini, bool permissive)
		{
			// e.g. ini="enum!0~2!0,1,2";
			std::vector<std::string> vs = ParamUtil::Tokenize(ini, '!');
			assert(vs.size() == 3);
			ParamUtil::ParseRange(vs[1], &mVcl, &mVch);
			std::vector<std::string> ve = ParamUtil::Tokenize(vs[2], ',');
			assert(!ve.empty());
			mIndices.resize(ve.size());
			for(size_t c = mIndices.size(), i = 0; i < c; i ++) mIndices[i] = atoi(ve[i].c_str());
			mMap.Setup(mVcl, mVch, 0, (float)(mIndices.size() - 1));
			mPermissive = permissive;
		}
		virtual bool ControlToNative(float vc, float* pvn) const
		{
			if(mPermissive) vc = ParamUtil::Limit(mVcl, mVch, vc);
			size_t i = (size_t)(mMap.Map(vc) + 0.5f);
			if(i < mIndices.size()) { *pvn = (float)mIndices[i]; return true; }
			return false;
		}
		virtual bool NativeToControl(float vn, float* pvc) const
		{
			size_t i = std::distance(mIndices.begin(), std::find(mIndices.begin(), mIndices.end(), (int)vn));
			if((mIndices.size() <= i) && mPermissive) i = mIndices.size() - 1;
			if(i < mIndices.size()) { *pvc = mMap.Unmap((float)i); return true; }
			return false;
		}
	};

	class LinValueConverter : public IValueConverter
	{
	public:
		float mVcl, mVch, mVnl, mVnh;
		CurveMapLinearF mMap;
		bool mPermissive;
		LinValueConverter(const std::string& ini, bool permissive)
		{
			// e.g. ini="lin!0~1!0~1";
			std::vector<std::string> vs = ParamUtil::Tokenize(ini, '!');
			assert(vs.size() == 3);
			ParamUtil::ParseRange(vs[1], &mVcl, &mVch);
			ParamUtil::ParseRange(vs[2], &mVnl, &mVnh);
			mMap.Setup(mVcl, mVch, mVnl, mVnh);
			mPermissive = permissive;
		}
		virtual bool ControlToNative(float vc, float* pvn) const
		{
			if(mPermissive || ((mVcl <= vc) && (vc <= mVch))) { *pvn = mMap.Map(vc); return true; }
			return false;
		}
		virtual bool NativeToControl(float vn, float* pvc) const
		{
			if(mPermissive || ((mVnl <= vn) && (vn <= mVnh))) { *pvc = mMap.Unmap(vn); return true; }
			return false;
		}
	};

	class ExpValueConverter : public IValueConverter
	{
	public:
		float mVcl, mVch, mVnl, mVnh;
		CurveMapExponentialF mMap;
		bool mPermissive;
		ExpValueConverter(const std::string& ini, bool permissive)
		{
			// e.g. ini="exp!0~1!0.001~1";
			std::vector<std::string> vs = ParamUtil::Tokenize(ini, '!');
			assert(vs.size() == 3);
			ParamUtil::ParseRange(vs[1], &mVcl, &mVch);
			ParamUtil::ParseRange(vs[2], &mVnl, &mVnh);
			mMap.Setup(mVcl, mVch, mVnl, mVnh);
			mPermissive = permissive;
		}
		virtual bool ControlToNative(float vc, float* pvn) const
		{
			if(mPermissive || ((mVcl <= vc) && (vc <= mVch))) { *pvn = mMap.Map(vc); return true; }
			return false;
		}
		virtual bool NativeToControl(float vn, float* pvc) const
		{
			if(mPermissive || ((mVnl <= vn) && (vn <= mVnh))) { *pvc = mMap.Unmap(vn); return true; }
			return false;
		}
	};

	//==============================================================================
	// string converters

	class PtStringConverter : public IStringConverter
	{
	public:
		float mVc;
		std::string mF;
		bool mPermissive;
		PtStringConverter(const std::string& ini, bool permissive)
		{
			// e.g. ini="pt!0!Off";
			std::vector<std::string> vs = ParamUtil::Tokenize(ini, '!');
			assert(vs.size() == 3);
			ParamUtil::ParseValue(vs[1], &mVc);
			mF = ParamUtil::Trim(vs[2]);
			mPermissive = permissive;
		}
		virtual bool IsEnum() const { return true; }
		virtual int GetEnumCount() const { return 1; }
		virtual std::vector<std::string> GetEnumStrings() const { return std::vector<std::string>(1, mF); }
		virtual bool ControlToEnumIndex(float vc, int* pi) const
		{
			if(mPermissive) vc = mVc;
			if(vc == mVc) { *pi = 0; return true; }
			return false;
		}
		virtual bool EnumIndexToControl(int i, float* pvc) const
		{
			if(mPermissive || (i == 0)) { *pvc = mVc; return true; }
			return false;
		}
		virtual bool Format(float vc, std::string* ps) const
		{
			if(mPermissive) vc = mVc;
			if(vc == mVc) { *ps = mF; return true; }
			return false;
		}
		virtual bool Parse(const std::string& s, float* pvc) const
		{
			if(mPermissive || (strcasecmp(s.c_str(), mF.c_str()) == 0)) { *pvc = mVc; return true; }
			return false;
		}
	};

	class EnumStringConverter : public IStringConverter
	{
	public:
		float mVcl, mVch;
		std::vector<std::string> mFormats;
		CurveMapLinearF mMap;
		bool mPermissive;
		EnumStringConverter(const std::string& ini, bool permissive)
		{
			// e.g. ini="enum!0~1!saw,tri,rect";
			std::vector<std::string> vs = ParamUtil::Tokenize(ini, '!');
			assert(vs.size() == 3);
			ParamUtil::ParseRange(vs[1], &mVcl, &mVch);
			mFormats = ParamUtil::Tokenize(vs[2], ',');
			assert(!mFormats.empty());
			for(std::vector<std::string>::iterator i = mFormats.begin(); i != mFormats.end(); i ++) *i = ParamUtil::Trim(*i);
			mMap.Setup(mVcl, mVch, 0, (float)(mFormats.size() - 1));
			mPermissive = permissive;
		}
		virtual bool IsEnum() const { return true; }
		virtual int GetEnumCount() const { return (int)mFormats.size(); }
		virtual std::vector<std::string> GetEnumStrings() const { return mFormats; }
		virtual bool ControlToEnumIndex(float vc, int* pi) const
		{
			if(mPermissive) vc = ParamUtil::Limit(mVcl, mVch, vc);
			size_t i = (size_t)(mMap.Map(vc) + 0.5f);
			if(i < mFormats.size()) { *pi = (int)i; return true; }
			return false;
		}
		virtual bool EnumIndexToControl(int i, float* pvc) const
		{
			size_t ii = (size_t)i;
			if(mPermissive) ii = ParamUtil::Limit((size_t)0, mFormats.size() - 1, ii);
			if(ii < mFormats.size()) { *pvc = mMap.Unmap((float)ii); return true; }
			return false;
		}
		virtual bool Format(float vc, std::string* ps) const
		{
			if(mPermissive) vc = ParamUtil::Limit(mVcl, mVch, vc);
			size_t i = (size_t)(mMap.Map(vc) + 0.5f);
			if(i < mFormats.size()) { *ps = mFormats[i]; return true; }
			return false;
		}
		virtual bool Parse(const std::string& s, float* pvc) const
		{
			size_t i = ParamUtil::FindMostMatch(mFormats, s);
			if(mPermissive) i = ParamUtil::Limit((size_t)0, mFormats.size() - 1, i);
			if(i < mFormats.size()) { *pvc = mMap.Unmap((float)i); return true; }
			return false;
		}
	};

	class LinStringConverter : public IStringConverter
	{
	public:
		float mVcl, mVch, mVnl, mVnh;
		PrintFormat mPrintFormat; // "format,param1,param2,..." : countof(params)<=4
		ScanFormat mScanFormat; // "format,param"
		CurveMapLinearF mMap;
		bool mPermissive;
		LinStringConverter(const std::string& ini, bool permissive)
		{
			// e.g. ini="lin!0~1!0~100!%.3f:%.3f,x,100-x!%f,x";
			std::vector<std::string> vs = ParamUtil::Tokenize(ini, '!');
			assert(vs.size() == 5);
			ParamUtil::ParseRange(vs[1], &mVcl, &mVch);
			ParamUtil::ParseRange(vs[2], &mVnl, &mVnh);
			std::vector<std::string> pfmt = ParamUtil::Tokenize(vs[3], ',');
			assert(2 <= pfmt.size());
			mPrintFormat.Setup(pfmt[0], std::vector<std::string>(std::next(pfmt.begin()), pfmt.end()));
			std::vector<std::string> sfmt = ParamUtil::Tokenize(vs[4], ',');
			assert(sfmt.size() == 2);
			mScanFormat.Setup(sfmt[0], sfmt[1]);
			mMap.Setup(mVcl, mVch, mVnl, mVnh);
			mPermissive = permissive;
		}
		virtual bool IsEnum() const { return false; }
		virtual int GetEnumCount() const { return 0; }
		virtual std::vector<std::string> GetEnumStrings() const { return std::vector<std::string>(); }
		virtual bool ControlToEnumIndex(float, int*) const { return false; }
		virtual bool EnumIndexToControl(int, float*) const { return false; }
		virtual bool Format(float vc, std::string* ps) const
		{
			if(mPermissive || ((mVcl <= vc) && (vc <= mVch)))
			{
				float v = mMap.Map(vc);
				*ps = mPrintFormat.Print(v);
				return true;
			}
			return false;
		}
		virtual bool Parse(const std::string& s, float* pvc) const
		{
			float vx = 0; if(mScanFormat.Scan(s, &vx))
			{
				if(mPermissive || ((mVnl <= vx) && (vx <= mVnh)))
				{
					*pvc = mMap.Unmap(vx);
					return true;
				}
			}
			return false;
		}
	};

	class ExpStringConverter : public IStringConverter
	{
	public:
		float mVcl, mVch, mVnl, mVnh;
		PrintFormat mPrintFormat; // "format,param1,param2,...", : countof(params)<=4
		ScanFormat mScanFormat; // "format,param"
		CurveMapExponentialF mMap;
		bool mPermissive;
		ExpStringConverter(const std::string& ini, bool permissive)
		{
			// e.g. ini="exp!0~1!0.001~1!%.3f:%.3f,x,1-x!%f,x";
			std::vector<std::string> vs = ParamUtil::Tokenize(ini, '!');
			assert(vs.size() == 5);
			ParamUtil::ParseRange(vs[1], &mVcl, &mVch);
			ParamUtil::ParseRange(vs[2], &mVnl, &mVnh);
			std::vector<std::string> pfmt = ParamUtil::Tokenize(vs[3], ',');
			assert(2 <= pfmt.size());
			mPrintFormat.Setup(pfmt[0], std::vector<std::string>(std::next(pfmt.begin()), pfmt.end()));
			std::vector<std::string> sfmt = ParamUtil::Tokenize(vs[4], ',');
			assert(sfmt.size() == 2);
			mScanFormat.Setup(sfmt[0], sfmt[1]);
			mMap.Setup(mVcl, mVch, mVnl, mVnh);
			mPermissive = permissive;
		}
		virtual bool IsEnum() const { return false; }
		virtual int GetEnumCount() const { return 0; }
		virtual std::vector<std::string> GetEnumStrings() const { return std::vector<std::string>(); }
		virtual bool ControlToEnumIndex(float, int*) const { return false; }
		virtual bool EnumIndexToControl(int, float*) const { return false; }
		virtual bool Format(float vc, std::string* ps) const
		{
			if(mPermissive || ((mVcl <= vc) && (vc <= mVch)))
			{
				float v = mMap.Map(vc);
				*ps = mPrintFormat.Print(v);
				return true;
			}
			return false;
		}
		virtual bool Parse(const std::string& s, float* pvc) const
		{
			float vx = 0; if(mScanFormat.Scan(s, &vx) == 1)
			{
				if(mPermissive || ((mVnl <= vx) && (vx <= mVnh)))
				{
					*pvc = mMap.Unmap(vx);
					return true;
				}
			}
			return false;
		}
	};

	//==============================================================================
	// ParamConverter

	ParamConverter::ParamConverter(const std::string& s)
	{
		Load(s);
	}

	ParamConverter::~ParamConverter()
	{
		Clear();
	}

	void ParamConverter::Load(const std::string& s)
	{
		Clear();
		// e.g. s="Param1" "\t" "Balance;%" "\t" "0~1;0.5" "\t" "lin!0~1!-1~1" "\t" "lin!0~1!0~100!%3.0f:%3.0f,x,100-x!x"
		std::vector<std::string> vs = ParamUtil::Tokenize(s, '\t');
		assert(vs.size() == 5);
		// key
		mKey = vs[0];
		// name, unit
		std::vector<std::string> nu = ParamUtil::Tokenize(vs[1], ';');
		mName = nu[0];
		if(1 < nu.size()) mUnit = nu[1];
		// value converter
		std::vector<std::string> vvc = ParamUtil::Tokenize(vs[3], ';');
		for(size_t c = vvc.size(), i = 0; i < c; i ++)
		{
			std::string ini = ParamUtil::Trim(vvc[i]);
			bool last = (i == (c - 1)) ? true : false;
			if     (strncasecmp(ini.c_str(), "pt"  , 2) == 0) mValConv.push_back(std::unique_ptr<IValueConverter>(new PtValueConverter(ini, last)));
			else if(strncasecmp(ini.c_str(), "enum", 4) == 0) mValConv.push_back(std::unique_ptr<IValueConverter>(new EnumValueConverter(ini, last)));
			else if(strncasecmp(ini.c_str(), "exp" , 3) == 0) mValConv.push_back(std::unique_ptr<IValueConverter>(new ExpValueConverter(ini, last)));
			else											  mValConv.push_back(std::unique_ptr<IValueConverter>(new LinValueConverter(ini, last)));
		}
		// minimum, maximum, default
		// note: to support the 'N' prefix, it must be parsed after the value converters
		std::vector<std::string> vp = ParamUtil::Tokenize(vs[2], ';');
		assert(2 <= vp.size());
		ParamUtil::ParseRange(vp[0], &mVcMin, &mVcMax);
		const std::string& vdef = ParamUtil::Trim(vp[1]);
		assert(1 <= vdef.size());
		if(vdef.front() == 'N')
		{
			float ndef = 0; ParamUtil::ParseValue(vdef.substr(1), &ndef);
			mVcDef = this->NativeToControl(ndef);
		}
		else ParamUtil::ParseValue(vdef, &mVcDef);
		if(3 <= vp.size())
		{
			std::string st = ParamUtil::Trim(vp[2]);
			if(strncasecmp(st.c_str(), "int", 3) == 0) mIsInteger = true;
		}
		// string converter
		std::vector<std::string> vsc = ParamUtil::Tokenize(vs[4], ';');
		for(size_t c = vsc.size(), i = 0; i < c; i ++)
		{
			std::string ini = ParamUtil::Trim(vsc[i]);
			bool last = (i == (c - 1)) ? true : false;
			if     (strncasecmp(ini.c_str(), "pt"  , 2) == 0) mStrConv.push_back(std::unique_ptr<IStringConverter>(new PtStringConverter(ini, last)));
			else if(strncasecmp(ini.c_str(), "enum", 4) == 0) mStrConv.push_back(std::unique_ptr<IStringConverter>(new EnumStringConverter(ini, last)));
			else if(strncasecmp(ini.c_str(), "exp" , 3) == 0) mStrConv.push_back(std::unique_ptr<IStringConverter>(new ExpStringConverter(ini, last)));
			else											  mStrConv.push_back(std::unique_ptr<IStringConverter>(new LinStringConverter(ini, last)));
		}
		mIsEnum = true;
		for(const auto& sc : mStrConv) if(!sc->IsEnum()) { mIsEnum = false; break; }
		if(mIsEnum) mIsInteger = true;
	}

	void ParamConverter::Clear()
	{
		mKey = mName = mUnit = "";
		mVcMin = mVcMax = mVcDef = 0;
		mIsEnum = false;
		mIsInteger = false;
		mValConv.clear();
		mStrConv.clear();
	}

	const char* ParamConverter::Key() const { return mKey.c_str(); }
	const char* ParamConverter::Name() const { return mName.c_str(); }
	const char* ParamConverter::Unit() const { return mUnit.c_str(); }
	float ParamConverter::ControlMin() const { return mVcMin; }
	float ParamConverter::ControlMax() const { return mVcMax; }
	float ParamConverter::ControlDef() const { return mVcDef; }
	float ParamConverter::LimitControlValue(float vc) const { return ParamUtil::Limit(mVcMin, mVcMax, vc); }

	float ParamConverter::NativeMin() const { return ControlToNative(mVcMin); }
	float ParamConverter::NativeMax() const { return ControlToNative(mVcMax); }
	float ParamConverter::NativeDef() const { return ControlToNative(mVcDef); }
	float ParamConverter::LimitNativeValue(float vn) const { return ParamUtil::LimitAbs(NativeMin(), NativeMax(), vn); }

	float ParamConverter::ControlToNative(float vc) const
	{
		for(size_t c = mValConv.size(), i = 0; i < c; i ++) { float v; if(mValConv[i]->ControlToNative(vc, &v)) return v; }
		return 0; // REVIEW: findout more appropriate value as default
	}

	float ParamConverter::NativeToControl(float vn) const
	{
		for(size_t c = mValConv.size(), i = 0; i < c; i ++) { float p; if(mValConv[i]->NativeToControl(vn, &p)) return p; }
		return mVcDef;
	}

	bool ParamConverter::IsInteger() const
	{
		return mIsInteger;
	}

	int ParamConverter::ControlToNativeInt(float vc) const
	{
		float vn = ControlToNative(vc);
		return (int)((0 <= vn) ? (vn + 0.5f) : (vn - 0.5f));
	}

	float ParamConverter::NativeIntToControl(int vn) const
	{
		return NativeToControl((float)vn);
	}

	bool ParamConverter::IsEnum() const
	{
		return mIsEnum;
	}

	int ParamConverter::GetEnumCount() const
	{
		int n = 0; for(size_t c = mStrConv.size(), i = 0; i < c; i ++) n += mStrConv[i]->GetEnumCount();
		return n;
	}

	std::vector<std::string> ParamConverter::GetEnumStrings() const
	{
		std::vector<std::string> vs;
		for(size_t c = mStrConv.size(), i = 0; i < c; i ++)
		{
			std::vector<std::string> v = mStrConv[i]->GetEnumStrings();
			vs.insert(vs.end(), v.begin(), v.end());
		}
		return vs;
	}

	int ParamConverter::ControlToEnumIndex(float vc) const
	{
		int offset = 0;
		for(size_t c = mStrConv.size(), i = 0; i < c; i ++)
		{
			int ii; if(mStrConv[i]->ControlToEnumIndex(vc, &ii)) return offset + ii;
			offset += mStrConv[i]->GetEnumCount();
		}
		return 0; // REVIEW: findout more appropriate value as default
	}

	float ParamConverter::EnumIndexToControl(int ii) const
	{
		int offset = 0;
		for(size_t c = mStrConv.size(), i = 0; i < c; i ++)
		{
			float p; if(mStrConv[i]->EnumIndexToControl(ii - offset, &p)) return ParamUtil::Limit(mVcMin, mVcMax, p);
			offset += mStrConv[i]->GetEnumCount();
		}
		return mVcDef;
	}

	std::string ParamConverter::Format(float vc) const
	{
		for(size_t c = mStrConv.size(), i = 0; i < c; i ++) { std::string s; if(mStrConv[i]->Format(vc, &s)) return s; }
		return std::string();
	}

	float ParamConverter::Parse(const std::string& s) const
	{
		for(size_t c = mStrConv.size(), i = 0; i < c; i ++) { float p; if(mStrConv[i]->Parse(s, &p)) return ParamUtil::Limit(mVcMin, mVcMax, p); }
		return mVcDef;
	}

	//==============================================================================
	// ParamConverterTable

	ParamConverterTable::ParamConverterTable()
	{
	}

	ParamConverterTable::ParamConverterTable(const char* const* ps, size_t cs)
	{
		Load(ps, cs);
	}

	ParamConverterTable::ParamConverterTable(const char* s)
	{
		Load(s);
	}

	ParamConverterTable::ParamConverterTable(const ParamConverterTable& r)
	{
		mTable = r.mTable;
		rebuildmap();
	}

	ParamConverterTable& ParamConverterTable::operator=(const ParamConverterTable& r)
	{
		mTable = r.mTable;
		rebuildmap();
		return *this;
	}

	ParamConverterTable::ParamConverterTable(ParamConverterTable&& r)
	{
		mTable = std::move(r.mTable);
		rebuildmap();
	}

	ParamConverterTable& ParamConverterTable::operator=(ParamConverterTable&& r)
	{
		mTable = std::move(r.mTable);
		rebuildmap();
		return *this;
	}

	ParamConverterTable::~ParamConverterTable()
	{
		Clear();
	}

	void ParamConverterTable::rebuildmap()
	{
		mMap.clear();
		for(const auto& m : mTable) { if(m->Key()[0]) mMap[m->Key()] = m.get(); }
	}

	void ParamConverterTable::Load(const char* const* ps, size_t cs)
	{
		assert(ps && cs);
		Clear();
		mTable.reserve(cs);
		while(cs --) mTable.push_back(std::make_shared<ParamConverter>(*ps ++));
		rebuildmap();
	}

	void ParamConverterTable::Load(const char* s)
	{
		assert(s);
		Clear();
		for(auto line : ParamUtil::Tokenize(s, "\r\n")) mTable.push_back(std::make_shared<ParamConverter>(line));
		rebuildmap();
	}

	void ParamConverterTable::Clear()
	{
		mTable.clear();
		mMap.clear();
	}

	void ParamConverterTable::replaceWith(size_t i, std::shared_ptr<ParamConverter> cvt)
	{
		ParamConverter* prev = mTable[i].get();
		auto im = std::find_if(mMap.begin(), mMap.end(), [prev](const std::pair<std::string, ParamConverter*>& e) { return e.second == prev; });
		if(im != mMap.end()) mMap.erase(im);
		mTable[i] = cvt;
		if(cvt->Key()[0]) mMap[cvt->Key()] = cvt.get();
	}

	size_t ParamConverterTable::Count() const { return mTable.size(); }

	const ParamConverter* ParamConverterTable::At(size_t i) const { return mTable[i].get(); }
	const ParamConverter* ParamConverterTable::operator[](size_t i) const { return mTable[i].get(); }
	const ParamConverter* ParamConverterTable::At(const char* k) const { auto it = mMap.find(k ? k : ""); return (it != mMap.end()) ? it->second : nullptr; }
	const ParamConverter* ParamConverterTable::operator[](const char* k) const { auto it = mMap.find(k ? k : ""); return (it != mMap.end()) ? it->second : nullptr; }

	ParamConverter* ParamConverterTable::At(size_t i) { return mTable[i].get(); }
	ParamConverter* ParamConverterTable::operator[](size_t i) { return mTable[i].get(); }
	ParamConverter* ParamConverterTable::At(const char* k) { auto it = mMap.find(k ? k : ""); return (it != mMap.end()) ? it->second : nullptr; }
	ParamConverter* ParamConverterTable::operator[](const char* k) { auto it = mMap.find(k ? k : ""); return (it != mMap.end()) ? it->second : nullptr; }

} // namespace FABB
