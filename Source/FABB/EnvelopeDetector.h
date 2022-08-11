//
//  EnvelopeDetector.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2017-11-16
//  (c) 2017 yu2924
//

#pragma once

#include <cmath>
#include "ApproxCR.h"

namespace FABB
{

	template<typename T> class EnvelopeDetectorT
	{
	public:
		static constexpr T PI2() { return (T)6.283185307179586476925286766559; }
		FABB::LagFilterF mLag;
		// for performance reason, sets in frequencies instead of time-constants
		// fc = 1 / (2 * PI * T)
		T mAttackFc;
		T mReleaseFc;
		EnvelopeDetectorT()
		{
			mAttackFc = (T)0;
			mReleaseFc = (T)0;
		}
		void SetAttackTC(T v)
		{
			mAttackFc = (T)1 / (PI2() * v);
		}
		void SetReleaseTC(T v)
		{
			mReleaseFc = (T)1 / (PI2() * v);
		}
		void Reset()
		{
			mLag.Reset();
		}
		T GetLastValue() const
		{
			return mLag.GetLastValue();
		}
		T Process(T v)
		{
			v = std::abs(v);
			mLag.SetFreq((mLag.GetLastValue() <= v) ? mAttackFc : mReleaseFc);
			return mLag.Process(v);
		}
		// allows inplace (pi == po)
		void Process(const T* pi, T* po, size_t l)
		{
			while(l --) *po ++ = Process(*pi ++);
		}
		void Process(T* p, size_t l)
		{
			Process(p, p, l);
		}
	};

	using EnvelopeDetectorF = EnvelopeDetectorT<float>;
	using EnvelopeDetectorD = EnvelopeDetectorT<double>;

} // namespace FABB
