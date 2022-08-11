//
//  ApproxCR.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2012-01-09
//  (c) 2012-2015 yu2924
//

// roughly approximation of analog CR

#pragma once

#include <cmath>

namespace FABB
{

	//       1
	// fc=------
	//    2*PI*T
	//
	// the time constant: T=CR
	// charge   : vo(t)=vi(t)*(1-e^(-t/T)), try Maxima "wxplot2d([y:1-exp(-x/1)], [x,0,5])"
	// dischange: vo(t)=vi(t)*e^(-t/T),     try Maxima "wxplot2d([y:exp(-x/1)], [x,0,5])"
	// discharge value after time T is: e^-1=0.3678794=-8.6858896dB
	// NOTE: speaking about step responce, when TC==1, the step responce is perfect rectangular, and TC<1 is not practical...
	template<typename T> class LagFilterT
	{
	public:
		static constexpr T TwoPi() { return (T)6.283185307179586476925286766559; }
		T m2PIFreq;
		T mS;
		LagFilterT(T f = 0)
		{
			m2PIFreq = TwoPi() * f;
			mS = 0;
		}
		T GetFreq() const
		{
			return m2PIFreq / TwoPi();
		}
		// freqmax=0.5
		void SetFreq(T v)
		{
			m2PIFreq = TwoPi() * v;
		}
		// tcmin=1/PI, practically 1<tc
		void SetTC(T v)
		{
			m2PIFreq = 1 / v;
		}
		T GetLastValue() const
		{
			return mS;
		}
		void Reset(T v = 0)
		{
			mS = v;
		}
		T Process(T v)
		{
			mS = mS + (v - mS) * m2PIFreq;
			return mS;
		}
		// allows inplace (pd == ps)
		void Process(const T* ps, T* pd, size_t l)
		{
			while(l --) *pd ++ = Process(*ps ++);
		}
		void Process(T* p, size_t l)
		{
			Process(p, p, l);
		}
	};

	template<typename T> class LeadFilterT
	{
	public:
		static constexpr T TwoPi() { return (T)6.283185307179586476925286766559; }
		T m2PIFreq;
		T mS, mOut;
		LeadFilterT(T f = 0)
		{
			m2PIFreq = TwoPi() * f;
			mS = mOut = 0;
		}
		T GetFreq() const
		{
			return m2PIFreq / TwoPi();
		}
		// freqmax=0.5
		void SetFreq(T v)
		{
			m2PIFreq = TwoPi() * v;
		}
		// tcmin=1/PI
		void SetTC(T v)
		{
			m2PIFreq = 1 / v;
		}
		T GetLastValue() const
		{
			return mOut;
		}
		void Reset(T v = 0)
		{
			mS = mOut = v;
		}
		T Process(T v)
		{
			mOut = v - mS;
			mS = mS + (v - mS) * m2PIFreq;
			return mOut;
		}
		// allows inplace (pd == ps)
		void Process(const T* ps, T* pd, size_t l)
		{
			while(l --) *pd ++ = Process(*ps ++);
		}
		void Process(T* p, size_t l)
		{
			Process(p, p, l);
		}
	};

	// similar to lag filters but constant Gain*Bandwidth product, vary the flat gain with the cutoff frequency
	template<typename T> class LeakyIntegratorT
	{
	public:
		static constexpr T TwoPi() { return (T)6.283185307179586476925286766559; }
		T m2PIFreq;
		T mS;
		LeakyIntegratorT(T f = 0)
		{
			m2PIFreq = TwoPi() * f;
			Reset();
		}
		T Getfreq() const
		{
			return m2PIFreq / TwoPi();
		}
		void SetFreq(T v)
		{
			m2PIFreq = TwoPi() * v;
		}
		T GetLastValue() const
		{
			return mS;
		}
		void Reset()
		{
			mS = 0;
		}
		T Process(T v)
		{
			mS = mS + v - mS*m2PIFreq;
			return mS;
		}
		// allows inplace (pd == ps)
		void Process(const T* ps, T* pd, size_t l)
		{
			while(l --) *pd ++ = Process(*ps ++);
		}
		void Process(T* p, size_t l)
		{
			Process(p, p, l);
		}
	};

	using LagFilterF = LagFilterT<float>;
	using LagFilterD = LagFilterT<double>;
	using LeadFilterF = LeadFilterT<float>;
	using LeadFilterD = LeadFilterT<double>;
	using LeakyIntegratorF = LeakyIntegratorT<float>;
	using LeakyIntegratorD = LeakyIntegratorT<double>;

} // namespace FABB
