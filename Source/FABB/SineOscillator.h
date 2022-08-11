//
//  SineOscillator.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2017-11-13
//  (c) 2017 yu2924
//

#pragma once

#include <cmath>

namespace FABB
{

	template<typename T> class SineOscillatorT
	{
	public:
		static constexpr T TWOPI() { return (T)6.283185307179586476925286766559; }
		T mFreq;
		T mPhase; // [0~1]
		SineOscillatorT()
		{
			SetFreq((T)0.0001);
			Reset();
		}
		void SetFreq(T v)
		{
			mFreq = v;
		}
		void Reset()
		{
			mPhase = 0;
		}
		T Process()
		{
			T v = std::sin(mPhase * TWOPI());
			mPhase += mFreq; while(1 <= mPhase) mPhase -= 1;
			return v;
		}
		void Process(T* p, size_t l)
		{
			while(l --) *p ++ = Process();
		}
	};

	using SineOscillatorF = SineOscillatorT<float>;
	using SineOscillatorD = SineOscillatorT<double>;

} // namespace FABB
