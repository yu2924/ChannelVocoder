//
//  BLT.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2012-01-09
//  (c) 2012-2015 yu2924
//

#pragma once

#include <cmath>
#include "IIR.h"

namespace FABB
{

	class AFConst
	{
	public:
		template<typename T> static constexpr T TwoPi() { return (T)6.283185307179586476925286766559; }
		template<typename T> static constexpr T QDef() { return (T)0.70710678118654752440084436210485; } // 1/sqrt(2)
	};

	//
	//        b0 + b1*z^-1   b0*z + b1
	// H(z) = ------------ = ---------
	//         1 + a1*z~-1      z + a1
	//
	template<typename T, class TBaseIIR> class DCBlockerT : public TBaseIIR
	{
	public:
		static constexpr T DefaultRadius() { return (T)0.995; }
		T mR;
		void InternalUpdate()
		{
			TBaseIIR* p = (TBaseIIR*)this;
			TBaseIIR::Coef& coef = p->mCoef;
			/*
			DC1z=(z-1)/(z-R);
			a0=1;
			a1=-R;
			b0=1;
			b1=-1;
			*/
			coef.a1 = -mR;
			coef.b0 = 1;
			coef.b1 = -1;
		}
		DCBlockerT(T r = DefaultRadius()) : mR(r)
		{
			InternalUpdate();
		}
		T GetR() const
		{
			return mR;
		}
		void SetR(T v)
		{
			mR = v;
			InternalUpdate();
		}
	};

	//
	//        b0 + b1*z^-1   b0*z + b1
	// H(z) = ------------ = ---------
	//         1 + a1*z~-1      z + a1
	//
	template<typename T, class TBaseIIR> class Analog1FilterT : public TBaseIIR
	{
	public:
		enum Type { LP, HP, AP } mType;
		T mFreq;
		void InternalUpdate()
		{
			TBaseIIR* p = (TBaseIIR*)this;
			TBaseIIR::Coef& coef = p->mCoef;
			T w = AFConst::TwoPi<T>() * mFreq, c = std::cos(w), s = std::sin(w);
			T a0, rcpa0;
			switch(mType)
			{
				case LP:
				{
					/*
					Hs=1/(s+1);
					Hz=-((c-1)*z+c-1)/((s-c+1)*z-s-c+1);
					a0=s-c+1;
					a1=-s-c+1;
					b0=-c+1;
					b1=-c+1;
					*/
					a0 = (s - c + 1); rcpa0 = 1 / a0;
					coef.a1 = (-s - c + 1) * rcpa0;
					coef.b0 = (-c + 1) * rcpa0;
					coef.b1 = coef.b0;
					break;
				}
				case HP:
				{
					/*
					Hs=s/(s+1);
					Hz=(s*z-s)/((s-c+1)*z-s-c+1);
					a0=s-c+1;
					a1=-s-c+1;
					b0=s;
					b1=-s;
					*/
					a0 = (s - c + 1); rcpa0 = 1 / a0;
					coef.a1 = (-s - c + 1) * rcpa0;
					coef.b0 = (s)* rcpa0;
					coef.b1 = -coef.b0;
					break;
				}
				case AP:
				{
					/*
					Hs=(1-s)/(1+s);
					Hz=-((s+c-1)*z-s+c-1)/((s-c+1)*z-s-c+1);
					a0=s-c+1;
					a1=-s-c+1;
					b0=-s-c+1;
					b1=s-c+1;
					*/
					a0 = (s - c + 1); rcpa0 = 1 / a0;
					coef.a1 = (-s - c + 1) * rcpa0;
					coef.b0 = coef.a1;
					coef.b1 = 1;
					break;
				}
			}
		}
		Analog1FilterT(Type t = LP, T f = (T)0.25) : mType(t), mFreq(f)
		{
			InternalUpdate();
		}
		Type GetType() const
		{
			return mType;
		}
		void SetType(Type v)
		{
			mType = v;
			InternalUpdate();
		}
		T GetFreq() const
		{
			return mFreq;
		}
		void SetFreq(T v)
		{
			mFreq = v;
			InternalUpdate();
		}
	};

	//
	// based on: http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
	//
	//        b0 + b1*z^-1 + b2*z^-2   b0*Z^2 + b1*z + b2
	// H(z) = ---------------------- = ------------------
	//         1 + a1*z^-1 + a2*z^-2      Z^2 + a1*z + a2
	//
	template<typename T, class TBaseIIR> class RBJ2FilterT : public TBaseIIR
	{
	public:
		enum Type { LP, HP, BP, BR, AP, PE, LS, HS } mType;
		T mFreq;
		T mQ;
		T mA;
		void InternalUpdate()
		{
			TBaseIIR* p = (TBaseIIR*)this;
			TBaseIIR::Coef& coef = p->mCoef;
			T w = AFConst::TwoPi<T>() * mFreq, c = std::cos(w), s = std::sin(w);
			T a0, rcpa0;
			switch(mType)
			{
				case LP:
				{
					T alpha = s / (2 * mQ);
					a0 = 1 + alpha; rcpa0 = 1 / a0;
					coef.a1 = (-2 * c) * rcpa0;
					coef.a2 = (1 - alpha) * rcpa0;
					coef.b0 = ((1 - c)*0.5f) * rcpa0;
					coef.b1 = (1 - c) * rcpa0;
					coef.b2 = ((1 - c)*0.5f) * rcpa0;
					break;
				}
				case HP:
				{
					T alpha = s / (2 * mQ);
					a0 = 1 + alpha; rcpa0 = 1 / a0;
					coef.a1 = (-2 * c) * rcpa0;
					coef.a2 = (1 - alpha) * rcpa0;
					coef.b0 = ((1 + c)*0.5f) * rcpa0;
					coef.b1 = (-(1 + c)) * rcpa0;
					coef.b2 = ((1 + c)*0.5f) * rcpa0;
					break;
				}
				case BP:
				{
					T alpha = s / (2 * mQ);
					a0 = 1 + alpha; rcpa0 = 1 / a0;
					coef.a1 = (-2 * c) * rcpa0;
					coef.a2 = (1 - alpha) * rcpa0;
					coef.b0 = (alpha)* rcpa0;
					coef.b1 = (0) * rcpa0;
					coef.b2 = (-alpha) * rcpa0;
					break;
				}
				case BR:
				{
					T alpha = s / (2 * mQ);
					a0 = 1 + alpha; rcpa0 = 1 / a0;
					coef.a1 = (-2 * c) * rcpa0;
					coef.a2 = (1 - alpha) * rcpa0;
					coef.b0 = (1) * rcpa0;
					coef.b1 = (-2 * c) * rcpa0;
					coef.b2 = (1) * rcpa0;
					break;
				}
				case AP:
				{
					T alpha = s / (2 * mQ);
					a0 = 1 + alpha; rcpa0 = 1 / a0;
					coef.a1 = (-2 * c) * rcpa0;
					coef.a2 = (1 - alpha) * rcpa0;
					coef.b0 = (1 - alpha) * rcpa0;
					coef.b1 = (-2 * c) * rcpa0;
					coef.b2 = (1 + alpha) * rcpa0;
					break;
				}
				case PE:
				{
					T A = sqrt(mA);
					T alpha = s / (2 * mQ);
					a0 = 1 + (alpha / A); rcpa0 = 1 / a0;
					coef.a1 = (-2 * c) * rcpa0;
					coef.a2 = (1 - (alpha / A)) * rcpa0;
					coef.b0 = (1 + (alpha*A)) * rcpa0;
					coef.b1 = (-2 * c) * rcpa0;
					coef.b2 = (1 - (alpha*A)) * rcpa0;
					break;
				}
				case LS:
				{
					T A = sqrt(mA);
					T beta = sqrt(2 * A);
					a0 = (A + 1) + (A - 1)*c + beta*s; rcpa0 = 1 / a0;
					coef.a1 = (-2 * ((A - 1) + (A + 1)*c)) * rcpa0;
					coef.a2 = ((A + 1) + (A - 1)*c - beta*s) * rcpa0;
					coef.b0 = (A*((A + 1) - (A - 1)*c + beta*s)) * rcpa0;
					coef.b1 = (2 * A*((A - 1) - (A + 1)*c)) * rcpa0;
					coef.b2 = (A*((A + 1) - (A - 1)*c - beta*s)) * rcpa0;
					break;
				}
				case HS:
				{
					T A = sqrt(mA);
					T beta = sqrt(2 * A);
					a0 = (A + 1) - (A - 1)*c + beta*s; rcpa0 = 1 / a0;
					coef.a1 = (2 * ((A - 1) - (A + 1)*c)) * rcpa0;
					coef.a2 = ((A + 1) - (A - 1)*c - beta*s) * rcpa0;
					coef.b0 = (A*((A + 1) + (A - 1)*c + beta*s)) * rcpa0;
					coef.b1 = (-2 * A*((A - 1) + (A + 1)*c)) * rcpa0;
					coef.b2 = (A*((A + 1) + (A - 1)*c - beta*s)) * rcpa0;
					break;
				}
			}
		}
		RBJ2FilterT(Type t = LP, T f = 0.25f, T q = AFConst::QDef<T>(), T a = 1) : mType(t), mFreq(f), mQ(q), mA(a)
		{
			InternalUpdate();
		}
		Type GetType() const
		{
			return mType;
		}
		void SetType(Type v)
		{
			mType = v;
			InternalUpdate();
		}
		T GetFreq() const
		{
			return mFreq;
		}
		void SetFreq(T v)
		{
			mFreq = v;
			InternalUpdate();
		}
		T GetQ() const
		{
			return mQ;
		}
		void SetQ(T v)
		{
			mQ = v;
			InternalUpdate();
		}
		T GetA() const
		{
			return mA;
		}
		void SetA(T v)
		{
			mA = v;
			InternalUpdate();
		}
		void SetFQA(T f, T q, T a)
		{
			mFreq = f;
			mQ = q;
			mA = a;
			InternalUpdate();
		}
	};

	using DCBlockerF = DCBlockerT<float, IIR1F>;
	using DCBlockerD = DCBlockerT<double, IIR1D>;
	using Analog1FilterF = Analog1FilterT<float, IIR1F>;
	using Analog1FilterD = Analog1FilterT<double, IIR1D>;
	using RBJ2FilterF = RBJ2FilterT<float, IIR2F>;
	using RBJ2FilterD = RBJ2FilterT<double, IIR2D>;

} // namespace FABB
