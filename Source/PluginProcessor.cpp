//
//  PluginProcessor.cpp
//  ChannelVocoder_SharedCode
//
//  Created by yu2924 on 2017-11-12
//  (c) 2017 yu2924
//

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PulseInstrument.h"
#include "ChannelVocoder.h"
#include "FABB/EnvelopeFollower.h"
#include <array>

// ===============================================================================
// VocoderCore

static const char* const gParamProfile[] =
{
	//			 nameunit					 param			 valueconvert					 stringconvert
	"IPT"	"\t" "Portamento;sec."	"\t" "0~1;N0.01"	"\t" "exp!0~1!0.001~1"			"\t" "exp!0~1!0.001~1!%.3f,x!%f,x",
	"IAT"	"\t" "Attack;sec."		"\t" "0~1;N0.01"	"\t" "exp!0~1!0.001~1"			"\t" "exp!0~1!0.001~1!%.3f,x!%f,x",
	"IRT"	"\t" "Release;sec."		"\t" "0~1;N0.01"	"\t" "exp!0~1!0.001~1"			"\t" "exp!0~1!0.001~1!%.3f,x!%f,x",
	"ILR"	"\t" "LFORate;Hz"		"\t" "0~1;N5"		"\t" "exp!0~1!0.1~100"			"\t" "exp!0~1!0.1~100!%.2f,x!%f,x",
	"IMR"	"\t" "ModRange"			"\t" "0~1;N2"		"\t" "lin!0~1!0~12"				"\t" "pt!0!Off; lin!0~1!0~12!%.2f,x!%f,x",
	"IBR"	"\t" "BendRange"		"\t" "0~1;N2"		"\t" "lin!0~1!0~12"				"\t" "pt!0!Off; lin!0~1!0~12!%.2f,x!%f,x",
	"IMM"	"\t" "Mode"				"\t" "0~1;N1"		"\t" "enum!0~1!0,1"				"\t" "enum!0~1!mono,poly",
	"CG"	"\t" "Carrier;dB"		"\t" "0~1;N2"		"\t" "pt!0!0; exp!0~1!0.1~10"	"\t" "pt!0!Off; lin!0~1!-20~20!%.1f,x!%f,x",
	"MG"	"\t" "Modulator;dB"		"\t" "0~1;N2"		"\t" "pt!0!0; exp!0~1!0.1~10"	"\t" "pt!0!Off; lin!0~1!-20~20!%.1f,x!%f,x",
	"OG"	"\t" "Output;dB"		"\t" "0~1;N2"		"\t" "pt!0!0; exp!0~1!0.1~10"	"\t" "pt!0!Off; lin!0~1!-20~20!%.1f,x!%f,x",
	"NG"	"\t" "Noise;dB"			"\t" "0~1;N0.5"		"\t" "pt!0!0; exp!0~1!0.1~10"	"\t" "pt!0!Off; lin!0~1!-20~20!%.1f,x!%f,x",
	"BS"	"\t" "Band Shift"		"\t" "0~1;N0;int"	"\t" "lin!0~1!-4~4"				"\t" "lin!0~1!-4~4!%.0f,x!%f,x",
};

class LevelMeter : public FABB::EnvelopeFollowerF
{
public:
	void ProcessWrite(const float* p, int l)
	{
		while(l --) Process(*p ++);
	}
};

class VocoderCore
{
public:
	CriticalSection mLock;
	FABB::ParamConverterTable mParamConverterTable;
	PulseInstrument mInstrument;
	ChannelVocoder mVocoder;
	std::array<LevelMeter, 3> mIOMeters;
	std::array<float, ParamID::Count> mChunk;
	float mCarrierGain, mModulatorGain, mOutputGain;
	int mNchC, mNchM, mNchO;
	VocoderCore()
	{
		mNchC = mNchM = mNchO = 0;
		mParamConverterTable.Load(gParamProfile, numElementsInArray(gParamProfile));
		jassert(mParamConverterTable.Count() == ParamID::Count);
		for(int ip = 0; ip < ParamID::Count; ip ++)
		{
			const FABB::ParamConverter* pc = mParamConverterTable[ip];
			mChunk[ip] = pc->ControlDef();
			SetParam(ip, mChunk[ip]);
		}
	}
	const FABB::ParamConverter* GetParamConverter(int ip) const
	{
		return mParamConverterTable[ip];
	}
	float GetParam(int ip) const
	{
		return mChunk[ip];
	}
	void SetParam(int ip, float v)
	{
		ScopedLock sl(mLock);
		mChunk[ip] = v;
		const FABB::ParamConverter* pc = mParamConverterTable[ip];
		switch(ip)
		{
			case ParamID::InstPortamentoTime: mInstrument.SetPortamentoTime(pc->ControlToNative(v)); break;
			case ParamID::InstAttackTime: mInstrument.SetAttackTime(pc->ControlToNative(v)); break;
			case ParamID::InstReleaseTime: mInstrument.SetReleaseTime(pc->ControlToNative(v)); break;
			case ParamID::InstLFORate: mInstrument.SetLFORate(pc->ControlToNative(v)); break;
			case ParamID::InstModRange: mInstrument.SetModRange(pc->ControlToNative(v)); break;
			case ParamID::InstBendRange: mInstrument.SetBendRange(pc->ControlToNative(v)); break;
			case ParamID::InstMonoMode: mInstrument.setMonoMode(pc->ControlToEnumIndex(v) == 0); break;
			case ParamID::IOCarrierGain: mCarrierGain = pc->ControlToNative(v); break;
			case ParamID::IOModulatorGain: mModulatorGain = pc->ControlToNative(v); break;
			case ParamID::IOOutputGain: mOutputGain = pc->ControlToNative(v); break;
			case ParamID::VocNoiseGain: mVocoder.setNoiseGain(pc->ControlToNative(v)); break;
			case ParamID::VocBandShift: mVocoder.SetBandShift(pc->ControlToNativeInt(v)); break;
		}
	}
	void Prepare(double fs, int nchc, int nchm, int ncho)
	{
		mNchC = nchc;
		mNchM = nchm;
		mNchO = ncho;
		mInstrument.Prepare(fs);
		mVocoder.Prepare(fs);
		for(auto&& lv : mIOMeters)
		{
			lv.SetAttackTC(0.01f * (float)fs);
			lv.SetReleaseTC(0.1f * (float)fs);
		}
	}
	void Unprepare()
	{
		mInstrument.Unprepare();
		mVocoder.Unprepare();
		mNchC = mNchM = mNchO = 0;
	}
	virtual void Process(AudioSampleBuffer& asb, MidiBuffer& mb)
	{
		ScopedLock sl(mLock);
		if((mNchC < 1) || (mNchM < 1) || (mNchO < 1) || (asb.getNumChannels() < (mNchC + mNchM)) || (asb.getNumChannels() < mNchO)) { return; }
		int ichc = 0;
		int ichm = ichc + mNchC;
		int icho = 0;
		int lenbuf = asb.getNumSamples();
		// mix carrier channels into ch0
		if(1 < mNchC) asb.addFrom(ichc, 0, asb, ichc + 1, 0, lenbuf);
		// mix rendered instrument
		int ismp = 0;
		for(const MidiMessageMetadata mm : mb)
		{
			mInstrument.ProcessAdd(asb.getWritePointer(ichc, ismp), mm.samplePosition - ismp);
			ismp = mm.samplePosition;
			// DBG(String::toHexString(mm.data, mm.numBytes));
			switch(mm.data[0] & 0xf0U)
			{
				case 0x80U:
					mInstrument.NoteOff(mm.data[1]);
					break;
				case 0x90U:
					if(0 < mm.data[2]) mInstrument.NoteOn(mm.data[1]);
					else mInstrument.NoteOff(mm.data[1]);
					break;
				case 0xb0U:
					if(mm.data[1] == 1)
					{
						float vwh = (float)mm.data[2] / 127.0f;
						mInstrument.SetLFOModCtrl(vwh);
					}
					break;
				case 0xe0U:
				{
					int wh = (int)(((uint16)mm.data[2] << 7) | (uint16)mm.data[1]) - 8192;
					float vwh = (float)wh / 8192.0f;
					mInstrument.SetPitchBendCtrl(vwh);
					break;
				}
			}
		}
		if(ismp < lenbuf) mInstrument.ProcessAdd(asb.getWritePointer(ichc, ismp), lenbuf - ismp);
		asb.applyGain(ichc, 0, lenbuf, mCarrierGain);
		mIOMeters[0].ProcessWrite(asb.getReadPointer(ichc), lenbuf);
		// mix modulator channel into ch2
		if(1 < mNchM) asb.addFrom(ichm, 0, asb, ichm + 1, 0, lenbuf);
		asb.applyGain(ichm, 0, lenbuf, mModulatorGain);
		mIOMeters[1].ProcessWrite(asb.getReadPointer(ichm), lenbuf);
		// process vocoder
		mVocoder.Process(asb.getReadPointer(ichc), asb.getReadPointer(ichm), asb.getWritePointer(icho), lenbuf);
		// output
		asb.applyGain(icho, 0, lenbuf, mOutputGain);
		if(1 < mNchO) asb.copyFrom(icho + 1, 0, asb, icho, 0, lenbuf);
		mIOMeters[2].ProcessWrite(asb.getReadPointer(icho), lenbuf);
	}
	void GetLevels(VocoderAudioProcessor::Levels* pv) const
	{
		ScopedLock sl(mLock);
		for(size_t c = mIOMeters.size(), i = 0; i < c; i ++) pv->ios[i] = mIOMeters[i].GetValue();
		mVocoder.GetModLevels(&pv->modbands);
	}
};

// ===============================================================================
// VocoderAudioProcessor

class VocoderParameter : public AudioProcessorParameterWithID
{
public:
	VocoderCore* mCore;
	const FABB::ParamConverter* mParamConverter;
	int mIndex;
	VocoderParameter(VocoderCore* prc, int ip)
		: AudioProcessorParameterWithID(prc->GetParamConverter(ip)->Key(), prc->GetParamConverter(ip)->Name(), AudioProcessorParameterWithIDAttributes().withLabel(prc->GetParamConverter(ip)->Unit()))
		, mCore(prc)
		, mParamConverter(prc->GetParamConverter(ip))
		, mIndex(ip)
	{}
	virtual float getValue() const override { return mCore->GetParam(mIndex); }
	virtual void setValue(float v) override { mCore->SetParam(mIndex, v); }
	virtual float getDefaultValue() const override { return mParamConverter->ControlDef(); }
	virtual int getNumSteps() const override
	{
		if(mParamConverter->IsEnum()) return mParamConverter->GetEnumCount() - 1;
		if(mParamConverter->IsInteger()) return std::abs((int)mParamConverter->NativeMax() - (int)mParamConverter->NativeMin());
		return 0x7fffffff;
	}
	virtual bool isDiscrete() const override { return mParamConverter->IsEnum() || mParamConverter->IsInteger(); }
	virtual String getText(float v, int) const override { return mParamConverter->Format(v); }
	virtual float getValueForText(const String& s) const override { return mParamConverter->Parse(s.toStdString()); }
};

class VocoderAudioProcessorImpl : public VocoderAudioProcessor
{
private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderAudioProcessorImpl)
public:
	std::unique_ptr<VocoderCore> mCore;
	VocoderAudioProcessorImpl() : VocoderAudioProcessor(BusesProperties()
		.withInput("InstInput", AudioChannelSet::stereo(), true)
		.withInput("VoiceInput", AudioChannelSet::stereo(), true)
		.withOutput("Output", AudioChannelSet::stereo(), true))
	{
		mCore = std::make_unique<VocoderCore>();
		for(int ip = 0; ip < ParamID::Count; ip ++) addParameter(new VocoderParameter(mCore.get(), ip));
	}
	virtual ~VocoderAudioProcessorImpl()
	{
	}
	bool guessChannels(const BusesLayout& layouts, int* pcchcOrz, int* pcchmOrz, int* pcchoOrz) const
	{
		int cchc = 0, cchm = 0, ccho = 0;
		jassert(layouts.inputBuses.size() == 2);
		jassert(layouts.outputBuses.size() == 1);
		if(layouts.getNumChannels(true, 1) == 0)
		{
			if(layouts.getNumChannels(true, 0) < 2) return false;
			cchc = cchm = 1;
		}
		else
		{
			cchc = layouts.getNumChannels(true, 0);
			if(cchc < 1) return false;
			cchm = layouts.getNumChannels(true, 1);
			if(cchm < 1) return false;
		}
		ccho = layouts.getNumChannels(false, 0);
		if(ccho < 1) return false;
		if(pcchcOrz) *pcchcOrz = cchc;
		if(pcchmOrz) *pcchmOrz = cchm;
		if(pcchoOrz) *pcchoOrz = ccho;
		return true;
	}
#ifndef JucePlugin_PreferredChannelConfigurations
	virtual bool isBusesLayoutSupported(const BusesLayout& layouts) const override
	{
		DBG("isBusesLayoutSupported");
		for(int cb = layouts.inputBuses.size(), ib = 0; ib < cb; ++ ib)
		{
			DBG(String::formatted("  inputbus[%d]: cch=%d", ib, layouts.getNumChannels(true, ib)));
		}
		for(int cb = layouts.outputBuses.size(), ib = 0; ib < cb; ++ib)
		{
			DBG(String::formatted("  outputbus[%d]: cch=%d", ib, layouts.getNumChannels(false, ib)));
		}
		return guessChannels(layouts, nullptr, nullptr, nullptr);
	}
#endif
	// processing
	virtual void prepareToPlay(double fs, int) override
	{
		BusesLayout layouts = getBusesLayout();
		int cchc = 0, cchm = 0, ccho = 0;
		guessChannels(layouts, &cchc, &cchm, &ccho);
		DBG(String::formatted("[VocoderAudioProcessor] prepare carrier=%d modulator=%d output=%d", cchc, cchm, ccho));
		mCore->Prepare(fs, cchc, cchm, ccho);
	}
	virtual void releaseResources() override
	{
		mCore->Unprepare();
		// DBG("[VocoderAudioProcessor] release");
	}
	virtual void processBlock(AudioSampleBuffer& asb, MidiBuffer& mb) override
	{
		juce::ScopedNoDenormals noDenormals;
		mCore->Process(asb, mb);
	}
	// editor
	virtual AudioProcessorEditor* createEditor() override
	{
		return VocoderAudioProcessorEditor::createInstance(*this);
	}
	virtual bool hasEditor() const override
	{
		return true;
	}
	// attributes
	virtual const String getName() const override { return JucePlugin_Name; }
	virtual bool acceptsMidi() const override { return true; }
	virtual bool producesMidi() const override { return false; }
	virtual bool isMidiEffect() const override { return false; }
	virtual double getTailLengthSeconds() const override { return 0; }
	// persistences
	virtual int getNumPrograms() override { return 1; }
	virtual int getCurrentProgram() override { return 0; }
	virtual void setCurrentProgram(int) override {}
	virtual const String getProgramName(int) override { return {}; }
	virtual void changeProgramName(int, const String&) override {}
	virtual void getStateInformation(MemoryBlock&) override {}
	virtual void setStateInformation(const void*, int) override {}
	// external APIs
	void getLevels(Levels* pv) const { mCore->GetLevels(pv); }
};

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new VocoderAudioProcessorImpl;
}
