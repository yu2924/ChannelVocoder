//
//  BlitOscillator.h
//  Fundamental Audio Building Blocks
//
//  Created by yu2924 on 2017-11-13
//  (c) 2017 yu2924
//

#pragma once

#include <cmath>

namespace FABB
{

	// BLIT formula
	// http://musicdsp.org/files/waveforms.txt
	// 
	// blit=sin(PI*x*m/p)/(m*sin(PI*x/p));
	//   x: sample number ranging from 1 to period
	//   p: period in samples (fs/f0)
	//   m=2*((int)p/2)+1 (i.e. when p=odd, m=p otherwise m=p+1)
	template<typename T> class BlitOscillatorT
	{
	public:
		static constexpr T PI() { return (T)3.14159265358979323846; }
		static constexpr T EPS() { return (T)1.192092896e-07; }
		T mFreq;
		T mM, mRcpM;
		T mXxF; // x/p=x*freq
		BlitOscillatorT()
		{
			SetFreq((T)0.001);
			Reset();
		}
		void SetFreq(T v)
		{
			mFreq = v;
			T p = 1 / mFreq;
			mM = 2 * (T)((int)p / 2) + 1;
			mRcpM = 1 / mM;
		}
		void Reset()
		{
			mXxF = 0;
		}
		T Process()
		{
			T v = mRcpM * ((EPS() < mXxF) ? (std::sin(PI() * mM * mXxF) / std::sin(PI() * mXxF)) : 1);
			mXxF += mFreq;
			if(2 <= mXxF) mXxF -= 2;
			return v;
		}
		void Process(T* p, size_t l)
		{
			while(l --) *p ++ = Process();
		}
	};

	using BlitOscillatorF = BlitOscillatorT<float>;
	using BlitOscillatorD = BlitOscillatorT<double>;

} // namespace FABB
