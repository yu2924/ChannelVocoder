//
//  PluginEditor.h
//  ChannelVocoder_SharedCode
//
//  Created by yu2924 on 2017-11-12
//  (c) 2017 yu2924
//

#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"

class VocoderAudioProcessorEditor : public AudioProcessorEditor
{
protected:
	VocoderAudioProcessorEditor(VocoderAudioProcessor& v) : AudioProcessorEditor(v){}
public:
	static VocoderAudioProcessorEditor* createInstance(VocoderAudioProcessor& v);
};
