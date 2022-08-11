//
//  PluginProcessor.h
//  ChannelVocoder_SharedCode
//
//  Created by yu2924 on 2017-11-12
//  (c) 2017 yu2924
//

#pragma once

#include "JuceHeader.h"
#include "FABB/ParamConvert.h"
#include <array>

struct ParamID
{
	enum
	{
		// instrument
		InstPortamentoTime,
		InstAttackTime,
		InstReleaseTime,
		InstLFORate,
		InstModRange,
		InstBendRange,
		InstMonoMode,
		// io
		IOCarrierGain,
		IOModulatorGain,
		IOOutputGain,
		// vocoder
		VocNoiseGain,
		VocBandShift,
		Count,
	};
};

class VC1Core;

class VocoderAudioProcessor : public AudioProcessor
{
protected:
	VocoderAudioProcessor(const BusesProperties& bp) : AudioProcessor(bp) {}
public:
	// external APIs
	struct Levels { std::array<float, 3> ios; std::array<float, 16> modbands; };
	virtual void getLevels(Levels* pv) const = 0;
};
