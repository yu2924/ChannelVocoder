//
//  PulseInstrument.h
//  ChannelVocoder
//
//  Created by yu2924 on 2017-11-05
//  (c) 2017 yu2924
//

#ifndef PULSEINSTRUMENT_H_INCLUDED
#define PULSEINSTRUMENT_H_INCLUDED

#include "FABB/CurveMapping.h"
#include "FABB/ApproxCR.h"
#include "FABB/SineOscillator.h"
#include "FABB/BlitOscillator.h"
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <list>
#include <memory>

class EnvelopeAR
{
public:
	FABB::LagFilterF mLag;
	float mTA, mTR;
	float mGate;
	EnvelopeAR()
	{
		mTA = mTR = 1;
		mGate = 0;
	}
	void SetAttackTC(float v)
	{
		mTA = v;
	}
	void SetReleaseTC(float v)
	{
		mTR = v;
	}
	void Reset()
	{
		mGate = 0;
		mLag.Reset();
	}
	void GateOn()
	{
		mGate = 1;
		mLag.SetTC(std::max(1.0f, mTA));
	}
	void GateOff()
	{
		mGate = 0;
		mLag.SetTC(std::max(1.0f, mTR));
	}
	bool IsSounding() const
	{
		return (0 < mGate) || (0.001f <= mLag.GetLastValue());
	}
	float Process()
	{
		return mLag.Process(mGate);
	}
};

class PulseVoice
{
public:
	const FABB::CurveMapExponentialF& mPitchMap;
	FABB::BlitOscillatorF mOsc;
	EnvelopeAR mEnv;
	FABB::LagFilterF mPortaLag;
	int mNote;
	float mPitchMod;
	PulseVoice(const FABB::CurveMapExponentialF& pitchmap) : mPitchMap(pitchmap), mNote(-1)
	{
		SetPortamentoTC(1);
		SetAttackTC(1);
		SetReleaseTC(1);
		SetPitchMod(0);
		Reset();
	}
	void SetPortamentoTC(float v)
	{
		mPortaLag.SetTC(v);
	}
	void SetAttackTC(float v)
	{
		mEnv.SetAttackTC(v);
	}
	void SetReleaseTC(float v)
	{
		mEnv.SetReleaseTC(v);
	}
	void SetPitchMod(float v)
	{
		mPitchMod = v;
	}
	void Reset()
	{
		mOsc.Reset();
		mEnv.Reset();
		mNote = -1;
	}
	void NoteOn(int v, int vstart)
	{
		mNote = v;
		if(0 <= vstart) mPortaLag.Reset(mPitchMap.Map((float)vstart + mPitchMod));
		mEnv.GateOn();
	}
	void NoteOff()
	{
		mEnv.GateOff();
	}
	bool IsSounding() const
	{
		return mEnv.IsSounding();
	}
	int Note() const
	{
		return mNote;
	}
	float process()
	{
		mOsc.SetFreq(mPortaLag.Process(mPitchMap.Map((float)mNote + mPitchMod)));
		float v = mEnv.Process() * mOsc.Process();
		if(!mEnv.IsSounding()) mNote = -1;
		return v;
	}
};

class PulseInstrument
{
public:
	enum { NumVoices = 8, MonoMaxStack = 3 };
	using Voice = PulseVoice;
	using VoicePtr = std::shared_ptr<PulseVoice>;
	FABB::CurveMapExponentialF mPitchMap;
	FABB::SineOscillatorF mLFO;
	std::vector<VoicePtr> mVoices;
	std::vector<Voice*> mIdleVoices, mActiveVoices;
	std::vector<int> mNoteStack;
	float mPortamentTime, mAttackTime, mReleaseTime, mLFORate, mModRange, mBendRange;
	float mLFOModCtrl, mPitchBendCtrl;
	float mSampleRate;
	bool mMonoMode;
	PulseInstrument()
	{
		mPortamentTime = 0.01f;
		mAttackTime = 0.01f;
		mReleaseTime = 0.01f;
		mLFORate = 1.0f;
		mModRange = 2.0f;
		mBendRange = 2.0f;
		mLFOModCtrl = 0.0f;
		mPitchBendCtrl = 0.0f;
		mSampleRate = 44100.0f;
		mMonoMode = false;
		mVoices.reserve(NumVoices);
		mIdleVoices.reserve(NumVoices);
		mActiveVoices.reserve(NumVoices);
		for(int i = 0; i < NumVoices; i ++) mVoices.push_back(std::make_shared<PulseVoice>(mPitchMap));
		mNoteStack.reserve(MonoMaxStack + 1);
		Reset();
	}
	void SetPortamentoTime(float v)
	{
		mPortamentTime = v;
		for(auto&& voice : mVoices) voice->SetPortamentoTC(mPortamentTime * mSampleRate);
	}
	void SetAttackTime(float v)
	{
		mAttackTime = v;
		for(auto&& voice : mVoices) voice->SetAttackTC(mAttackTime * mSampleRate);
	}
	void SetReleaseTime(float v)
	{
		mReleaseTime = v;
		for(auto&& voice : mVoices) voice->SetReleaseTC(mReleaseTime * mSampleRate);
	}
	void SetLFORate(float v)
	{
		mLFORate = v;
		mLFO.SetFreq(mLFORate / mSampleRate);
	}
	void SetModRange(float v)
	{
		mModRange = v;
	}
	void SetBendRange(float v)
	{
		mBendRange = v;
	}
	void SetLFOModCtrl(float v)
	{
		mLFOModCtrl = v;
	}
	void SetPitchBendCtrl(float v)
	{
		mPitchBendCtrl = v;
	}
	void setMonoMode(bool v)
	{
		mMonoMode = v;
		Reset();
	}
	void Prepare(double fs)
	{
		mSampleRate = (float)fs;
		// fosc = 440 * 2^((note - 69) / 12)
		// note: A0:21	A1:33	A2:45	A3:57	A4:69	A5:81	A6:93	A7:105	A8:117	: in scientific pitch notation (SPN, c-1 ~ G9)
		// fosc: 27.5	55.0	110.0	220.0	440.0	880.0	1760.0	3520.0	7040.0
		mPitchMap.Setup(21, 117, 27.5f / mSampleRate, 7040.0f / mSampleRate);
		mLFO.SetFreq(mLFORate / mSampleRate);
		for(auto&& voice : mVoices)
		{
			voice->SetPortamentoTC(mPortamentTime * mSampleRate);
			voice->SetAttackTC(mAttackTime * mSampleRate);
			voice->SetReleaseTC(mReleaseTime * mSampleRate);
		}
		Reset();
	}
	void Unprepare()
	{
	}
	void Reset()
	{
		mLFO.Reset();
		mActiveVoices.resize(0);
		mIdleVoices.resize(0);
		for(auto&& voice : mVoices) mIdleVoices.push_back(voice.get());
		mNoteStack.resize(0);
	}
	void NoteOn(int v)
	{
		mNoteStack.erase(std::remove(mNoteStack.begin(), mNoteStack.end(), v), mNoteStack.end());
		mNoteStack.push_back(v);
		if(mMonoMode)
		{
			Voice* voice = nullptr;
			if(mActiveVoices.empty())
			{
				voice = mIdleVoices.front();
				mIdleVoices.erase(mIdleVoices.begin());
				mActiveVoices.push_back(voice);
			}
			else voice = mActiveVoices.front();
			voice->NoteOn(v, mNoteStack.empty() ? v : mNoteStack.back());
			if(MonoMaxStack < (int)mNoteStack.size()) mNoteStack.erase(mNoteStack.begin());
		}
		else
		{
			std::vector<Voice*>* arr = nullptr;
			std::vector<Voice*>::iterator it = std::find_if(mActiveVoices.begin(), mActiveVoices.end(), [v](const Voice* voice) { return voice->Note() == v; });
			if(it != mActiveVoices.end()) { arr = &mActiveVoices; }
			else if(mIdleVoices.empty()) { it = mActiveVoices.begin(); arr = &mActiveVoices; }
			else { it = mIdleVoices.begin(); arr = &mIdleVoices; }
			Voice* voice = *it;
			arr->erase(it);
			mActiveVoices.push_back(voice);
			voice->NoteOn(v, mNoteStack.empty() ? v : mNoteStack.back());
		}
	}
	void NoteOff(int v)
	{
		mNoteStack.erase(std::remove(mNoteStack.begin(), mNoteStack.end(), v), mNoteStack.end());
		if(mMonoMode)
		{
			if(!mActiveVoices.empty())
			{
				Voice* voice = mActiveVoices.front();
				if(!mNoteStack.empty()) voice->NoteOn(mNoteStack.back(), -1);
				else voice->NoteOff();
			}
		}
		else
		{
			auto it = std::find_if(mActiveVoices.begin(), mActiveVoices.end(), [v](const Voice* voice) { return voice->Note() == v; });
			if(it != mActiveVoices.end()) (*it)->NoteOff();
		}
	}
	float internalRawProcess()
	{
		float mod = mLFO.Process() * mModRange * mLFOModCtrl + mBendRange * mPitchBendCtrl;
		float v = 0;
		for(auto&& voice : mActiveVoices)
		{
			voice->SetPitchMod(mod);
			v += voice->process();
		}
		std::vector<Voice*>::iterator i = mActiveVoices.begin(); while(i != mActiveVoices.end())
		{
			if((*i)->IsSounding()) i ++;
			else { Voice* voice = *i; i = mActiveVoices.erase(i); mIdleVoices.push_back(voice); }
		}
		return v;
	}
	void Process(float* p, int l)
	{
		while(l --) *p ++ = internalRawProcess();
	}
	void ProcessAdd(float* p, int l)
	{
		while(l --) *p ++ += internalRawProcess();
	}
};

#endif  // PULSEINSTRUMENT_H_INCLUDED
