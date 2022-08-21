//
//  ChannelVocoder.h
//  ChannelVocoder
//
//  Created by yu2924 on 2017-11-05
//  (c) 2017 yu2924
//

#pragma once

#include "FABB/EnvelopeFollower.h"
#include "FABB/BLT.h"
#include <array>
#include <cstdint>

//
// for 1/3oct bands:
//   Q=8;
//   k=0.94;
//   g=2;
// fo=440*(2^([-5:10]/3))=[138.59132, 174.61412, 220, 277.18263, 349.22823, 440, 554.36526, 698.45646, 880, 1108.7305, 1396.9129, 1760, 2217.461, 2793.8259, 3520, 4434.9221]
// fo=500*(2^([-5:10]/3))=[157.49013, 198.42513, 250, 314.98026, 396.85026, 500, 629.96052, 793.70053, 1000, 1259.921, 1587.4011, 2000, 2519.8421, 3174.8021, 4000, 5039.6842]
//

// cascaded bandpass filters with staggered tuning
class CascadedBPF
{
public:
	static constexpr float Q() { return 8; }
	static constexpr float K() { return 0.94f; }
	static constexpr float G() { return 2; }
	FABB::RBJFilterF mFltA, mFltB;
	CascadedBPF()
	{
		mFltA.SetType(FABB::RBJFilterF::Type::BP);
		mFltB.SetType(FABB::RBJFilterF::Type::BP);
		mFltA.SetQ(Q());
		mFltB.SetQ(Q());
	}
	void SetFreq(float fo)
	{
		mFltA.SetFreq(fo * K());
		mFltB.SetFreq(fo / K());
	}
	void Reset()
	{
		mFltA.Reset();
		mFltB.Reset();
	}
	float Process(float v)
	{
		return mFltB.Process(mFltA.Process(v)) * G();
	}
};

// based on 'Pseudo-Random generator'
// http://musicdsp.org/archive.php?classid=1#59
// Reference: Hal Chamberlin, "Musical Applications of Microprocessors"
class NoiseGenerator
{
public:
	static constexpr float Scale() { return 1.0f / 2147483648.0f; }
	int32_t mIntValue;
	NoiseGenerator()
	{
		mIntValue = 22222;
	}
	float Process()
	{
		mIntValue = (mIntValue * 196314165) + 907633515;
		return (float)mIntValue * Scale();
	}
};

class ChannelVocoder
{
public:
	enum { BandCount = 16 };
	std::array<CascadedBPF, BandCount> mBPFC, mBPFM;
	std::array<FABB::EnvelopeFollowerF, BandCount> mEnvD;
	NoiseGenerator mNoiseGen;
	float mNoiseGain;
	int mBandShift;
	ChannelVocoder()
	{
		mNoiseGain = 0;
		mBandShift = 0;
	}
	void setNoiseGain(float v)
	{
		mNoiseGain = v;
	}
	void SetBandShift(int v)
	{
		mBandShift = v;
		Reset();
	}
	void Prepare(double fs)
	{
		float samplerate = (float)fs;
		for(int i = 0; i < BandCount; i ++)
		{
			// fo=500*(2^([-5:10]/3))
			float fo = 500 * std::pow(2.0f, (float)(i - 5) / 3.0f);
			mBPFC[i].SetFreq(fo / samplerate);
			mBPFM[i].SetFreq(fo / samplerate);
			mEnvD[i].SetAttackTC(0.01f * samplerate);
			mEnvD[i].SetReleaseTC(0.1f * samplerate);
		}
		Reset();
	}
	void Unprepare()
	{
	}
	void Reset()
	{
		for(int i = 0; i < BandCount; i ++)
		{
			mBPFC[i].Reset();
			mBPFM[i].Reset();
			mEnvD[i].Reset();
		}
	}
	void GetModLevels(std::array<float, BandCount>* pv) const
	{
		for(int i = 0; i < BandCount; i ++)
		{
			int im = i - mBandShift;
			pv->at(i) = ((0 <= im) && (im < BandCount)) ? mEnvD[im].GetValue() : 0;
		}
	}
	float Process(float vc, float vm)
	{
		static const float NoiseBands[BandCount] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
		float vn = mNoiseGen.Process() * mNoiseGain;
		float vo = 0;
		for(int i = 0; i < BandCount; i ++)
		{
			int im = i - mBandShift;
			float vmi = ((0 <= im) && (im < BandCount)) ? mEnvD[im].Process(mBPFM[im].Process(vm)) : 0;
			float vci = mBPFC[i].Process(vc + vn * NoiseBands[i]);
			vo += vmi * vci;
		}
		return vo;
	}
	void Process(const float* pc, const float* pm, float* po, int l)
	{
		while(l --) *po ++ = Process(*pc ++, *pm ++);
	}
};
