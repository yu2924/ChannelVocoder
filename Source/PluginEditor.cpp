//
//  PluginEditor.cpp
//  ChannelVocoder_SharedCode
//
//  Created by yu2924 on 2017-11-12
//  (c) 2017 yu2924
//

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FABB/CurveMapping.h"

// ===============================================================================
// LevelMeterPane

class LevelBar : public Component
{
public:
	Range<float> mRange;
	float mValue;
	bool mVertical;
	LevelBar()
		: mRange(0, 1)
		, mValue(0)
		, mVertical(true)
	{
	}
	virtual void paint(Graphics& g) override
	{
		Rectangle<float> rc = getLocalBounds().toFloat();
		g.setColour(Colours::black);
		g.fillRect(rc);
		rc.reduce(1, 1);
		g.setColour(Colours::lime);
		float ratio = (mValue - mRange.getStart()) / mRange.getLength();
		if(mVertical)
		{
			float cy = rc.getHeight(), cyb = cy * ratio;
			g.fillRect(Rectangle<float>(rc.getX(), rc.getY() + cy - cyb, rc.getWidth(), cyb));
		}
		else
		{
			float cx = rc.getWidth(), cxb = cx * ratio;
			g.fillRect(Rectangle<float>(rc.getX(), rc.getY(), cxb, rc.getHeight()));
		}
	}
	void setRange(const Range<float>& v)
	{
		mRange = v;
		repaint();
	}
	void setValue(float v)
	{
		mValue = v;
		repaint();
	}
	void setVertical(bool v)
	{
		mVertical = v;
		repaint();
	}
};

class LevelTickOverlay : public Component
{
public:
	Range<float> mRange;
	struct Tick { float value; String text; };
	std::vector<Tick> mTicks;
	float mMaxTextWidth;
	bool mVertical;
	Font mFont;
	static constexpr float FontHeight = 12;
	LevelTickOverlay()
		: mRange(0, 1)
		, mMaxTextWidth(0)
		, mVertical(true)
		, mFont(FontHeight, Font::FontStyleFlags::plain)
	{
		setInterceptsMouseClicks(false, false);
	}
	virtual void paint(Graphics& g) override
	{
		Colour clr = findColour(Label::ColourIds::textColourId);
		g.setColour(clr);
		g.setFont(mFont);
		if(mVertical)
		{
			Rectangle<int> rcc = getContentRect();
			FABB::CurveMapLinearF v2p(mRange.getStart(), mRange.getEnd(), (float)rcc.getBottom(), (float)rcc.getY());
			for(const auto& t : mTicks)
			{
				int y = roundToInt(v2p.Map(t.value));
				g.drawHorizontalLine(y, (float)rcc.getX(), (float)rcc.getRight());
				g.drawSingleLineText(t.text, rcc.getX(), y - roundToInt(FontHeight * 0.5f - mFont.getAscent()), Justification::right);
			}
		}
		else
		{
			Rectangle<int> rcc = getContentRect();
			FABB::CurveMapLinearF v2p(mRange.getStart(), mRange.getEnd(), (float)rcc.getX(), (float)rcc.getRight());
			for(const auto& t : mTicks)
			{
				int x = roundToInt(v2p.Map(t.value));
				g.drawVerticalLine(x, (float)rcc.getY(), (float)rcc.getBottom());
				g.drawSingleLineText(t.text, x, rcc.getY(), Justification::horizontallyCentred);
			}
		}
	}
	void setRange(const Range<float>& v)
	{
		mRange = v;
		repaint();
	}
	void setVertical(bool v)
	{
		mVertical = v;
		repaint();
	}
	void setTicks(const std::vector<Tick>& v)
	{
		mTicks = v;
		mMaxTextWidth = 0;
		Font font(FontHeight, Font::FontStyleFlags::plain);
		for(const auto& t : mTicks) mMaxTextWidth = std::max(mMaxTextWidth, font.getStringWidthFloat(t.text + " "));
		repaint();
	}
	Rectangle<int> getContentRect() const
	{
		Rectangle<int> rc = getLocalBounds();
		if(mVertical) return rc.withTrimmedLeft((int)(mMaxTextWidth + 1)).reduced(0, (int)(FontHeight * 0.5f + 1));
		else return rc.withTrimmedTop((int)(FontHeight + 1)).reduced((int)(mMaxTextWidth * 0.5f + 1), 0);
	}
};

class LevelMeterPane : public Component, public Timer
{
public:
	VocoderAudioProcessor* mProcessor;
	LevelTickOverlay mTickOverlay;
	struct Bar { LevelBar levelbar; Label label; };
	std::array<Bar, 3> mSigBars;
	std::array<Bar, 16> mChBars;
	enum { Margin = 8, SectionSpacing = 64, SigBarWidth = 32, ChBarWidth = 24, LevelBarWidth = 16, LabelHeight = 15 };
	LevelMeterPane(VocoderAudioProcessor* p)
		: mProcessor(p)
	{
		jassert(mProcessor != nullptr);
		static const String SigLabels[] = { "C", "M", "O" };
		for(size_t c = mSigBars.size(), i = 0; i < c; ++i)
		{
			mSigBars[i].label.setJustificationType(Justification::centred);
			mSigBars[i].label.setText(SigLabels[i], NotificationType::dontSendNotification);
			addAndMakeVisible(mSigBars[i].label);
			mSigBars[i].levelbar.setVertical(true);
			mSigBars[i].levelbar.setRange({ -40, 20 });
			mSigBars[i].levelbar.setValue(-40);
			addAndMakeVisible(mSigBars[i].levelbar);
		}
		for(size_t c = mChBars.size(), i = 0; i < c; ++i)
		{
			mChBars[i].label.setJustificationType(Justification::centred);
			mChBars[i].label.setText(String(i + 1), NotificationType::dontSendNotification);
			addAndMakeVisible(mChBars[i].label);
			mChBars[i].levelbar.setVertical(true);
			mChBars[i].levelbar.setRange({ -40, 20 });
			mChBars[i].levelbar.setValue(-40);
			addAndMakeVisible(mChBars[i].levelbar);
		}
		mTickOverlay.setVertical(true);
		mTickOverlay.setRange({ -40, 20 });
		static const std::vector<LevelTickOverlay::Tick> Ticks =
		{
			{  20.0f,  "20"},
			{   0.0f,   "0"},
			{ -20.0f, "-20"},
			{ -40.0f, "-40"},
		};
		mTickOverlay.setTicks(Ticks);
		addAndMakeVisible(mTickOverlay);
		startTimer(100);
	}
	virtual void resized() override
	{
		Rectangle<int> rc = getLocalBounds().reduced(Margin);
		mTickOverlay.setBounds(rc.withTrimmedBottom(LabelHeight));
		Rectangle<int> rcc = getLocalArea(&mTickOverlay, mTickOverlay.getContentRect());
		for(size_t c = mSigBars.size(), i = 0; i < c; ++i)
		{
			Rectangle<int> rcb = rcc.removeFromLeft(SigBarWidth);
			mSigBars[i].levelbar.setBounds(rcb.reduced((rcb.getWidth() - LevelBarWidth) / 2, 0));
			mSigBars[i].label.setBounds(rcb.getX(), rc.getBottom() - LabelHeight, SigBarWidth, LabelHeight);
		}
		rcc.removeFromLeft(SectionSpacing);
		for(size_t c = mChBars.size(), i = 0; i < c; ++i)
		{
			Rectangle<int> rcb = rcc.removeFromLeft(ChBarWidth);
			mChBars[i].levelbar.setBounds(rcb.reduced((rcb.getWidth() - LevelBarWidth) / 2, 0));
			mChBars[i].label.setBounds(rcb.getX(), rc.getBottom() - LabelHeight, ChBarWidth, LabelHeight);
		}
	}
	virtual void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xff202020));
	}
	// Timer
	virtual void timerCallback() override
	{
		VocoderAudioProcessor::Levels lv; mProcessor->getLevels(&lv);
		for(size_t c = mSigBars.size(), i = 0; i < c; ++i) mSigBars[i].levelbar.setValue(20 * std::log10(FLT_EPSILON + lv.ios[i]));
		for(size_t c = mChBars.size(), i = 0; i < c; ++i) mChBars[i].levelbar.setValue(20 * std::log10(FLT_EPSILON + lv.modbands[i]));
	}
};

// ===============================================================================
// ParamSectionPane

class ParamBoundSlider : public Component, protected Slider::Listener
{
public:
	AudioProcessorParameter* mParam;
	Label mLabel;
	Slider mSlider;
	enum { LabelHeight = 15 };
	ParamBoundSlider()
		: mParam(nullptr)
	{
		mLabel.setMinimumHorizontalScale(0.5f);
		mLabel.setJustificationType(Justification::centred);
		addAndMakeVisible(mLabel);
		mSlider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
		mSlider.addListener(this);
		mSlider.valueFromTextFunction = [this](const String& s)->double { return mParam ? mParam->getValueForText(s) : 0; };
		mSlider.textFromValueFunction = [this](double v)->String { return mParam ? mParam->getText((float)v, 256) : String("---"); };
		addAndMakeVisible(mSlider);
	}
	void bindToObject(AudioProcessorParameter* p)
	{
		mParam = p;
		if(mParam)
		{
			mLabel.setText(mParam->getName(256), NotificationType::dontSendNotification);
			double dv = mParam->isDiscrete() ? (1.0 / (double)mParam->getNumSteps()) : 0;
			mSlider.setRange(0, 1, dv);
			mSlider.setDoubleClickReturnValue(true, mParam->getDefaultValue());
			mSlider.setValue(mParam->getValue(), NotificationType::dontSendNotification);
		}
	}
	virtual void resized() override
	{
		Rectangle<int> rc = getLocalBounds();
		mLabel.setBounds(rc.removeFromBottom(LabelHeight));
		mSlider.setBounds(rc);
		mSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxAbove, false, rc.getWidth(), LabelHeight);
	}
	// Slider::Listener
	virtual void sliderValueChanged(Slider* slider) override
	{
		if(mParam) mParam->setValueNotifyingHost((float)slider->getValue());
	}
};

class ParamSectionPane : public GroupComponent
{
public:
	VocoderAudioProcessor* mProcessor;
	std::vector<int> mPIDs;
	OwnedArray<ParamBoundSlider> mSliders;
	enum { GroupMargin = 5, GroupLabelHeight = 15, KnobDiameter = 64, LabelHeight = 15, Spacing = 4 };
	ParamSectionPane(VocoderAudioProcessor* p, const std::vector<int>& pids, const String& title)
		: GroupComponent("", title)
		, mProcessor(p)
		, mPIDs(pids)
	{
		jassert(mProcessor != nullptr);
		const Array<AudioProcessorParameter*>& params = mProcessor->getParameters();
		mSliders.ensureStorageAllocated((int)mPIDs.size());
		for(size_t c = mPIDs.size(), i = 0; i < c; i ++)
		{
			AudioProcessorParameter* prm = params[(int)mPIDs[i]];
			ParamBoundSlider* slider = new ParamBoundSlider;
			slider->bindToObject(prm);
			addAndMakeVisible(slider);
			mSliders.add(slider);
		}
	}
	virtual void resized() override
	{
		Rectangle<int> rc = getLocalBounds().withTrimmedTop(GroupLabelHeight + Spacing).withTrimmedLeft(GroupMargin).withTrimmedRight(GroupMargin).withTrimmedBottom(GroupMargin);
		for(auto&& slider : mSliders)
		{
			slider->setBounds(rc.removeFromLeft(KnobDiameter));
			rc.removeFromLeft(Spacing);
		}
	}
	int getNaturalWidth() const
	{
		return GroupMargin * 2 + KnobDiameter * (int)mPIDs.size() + Spacing * (int)(mPIDs.size() - 1);
	}
	static int getNaturalHeight()
	{
		return GroupLabelHeight + GroupMargin + Spacing + LabelHeight * 2 + KnobDiameter;
	}
};

// ===============================================================================
// VocoderAudioProcessorEditor

class VocoderAudioProcessorEditorImpl : public VocoderAudioProcessorEditor
{
private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderAudioProcessorEditorImpl)
protected:
	VocoderAudioProcessor& processor;
	std::unique_ptr<ParamSectionPane> mSigSection;
	std::unique_ptr<ParamSectionPane> mVocSection;
	std::unique_ptr<ParamSectionPane> mInstSection;
	std::unique_ptr<LevelMeterPane> mLevelMeter;
	enum { Margin = 10, BarsHeight = 200 };
public:
	VocoderAudioProcessorEditorImpl(VocoderAudioProcessor& v) : VocoderAudioProcessorEditor(v), processor(v)
	{
		static const std::vector<int> IOPIDs = { ParamID::IOCarrierGain, ParamID::IOModulatorGain, ParamID::IOOutputGain };
		mSigSection = std::make_unique<ParamSectionPane>(&processor, IOPIDs, "Signal");
		addAndMakeVisible(mSigSection.get());
		static const std::vector<int> VOCPIDs = { ParamID::VocNoiseGain, ParamID::VocBandShift };
		mVocSection = std::make_unique<ParamSectionPane>(&processor, VOCPIDs, "Vocoder");
		addAndMakeVisible(mVocSection.get());
		static const std::vector<int> InstPIDs = { ParamID::InstPortamentoTime, ParamID::InstAttackTime, ParamID::InstReleaseTime, ParamID::InstLFORate, ParamID::InstModRange, ParamID::InstBendRange, ParamID::InstMonoMode };
		mInstSection = std::make_unique<ParamSectionPane>(&processor, InstPIDs, "Instrument");
		addAndMakeVisible(mInstSection.get());
		mLevelMeter = std::make_unique<LevelMeterPane>(&processor);
		addAndMakeVisible(mLevelMeter.get());
		int cyparams = ParamSectionPane::getNaturalHeight();
		int cxinst = mInstSection->getNaturalWidth();
		int cxvoc = mVocSection->getNaturalWidth();
		int cxio = mSigSection->getNaturalWidth();
		Rectangle<int> rc = { Margin * 2 + cxio + cxvoc + cxinst, Margin * 2 + BarsHeight + cyparams };
		Rectangle<int> rci = rc.reduced(Margin);
		mLevelMeter->setBounds(rci.removeFromTop(BarsHeight));
		mSigSection->setBounds(rci.removeFromLeft(cxio));
		mVocSection->setBounds(rci.removeFromLeft(cxvoc));
		mInstSection->setBounds(rci.removeFromLeft(cxinst));
		setSize(rc.getWidth(), rc.getHeight());
	}
	virtual ~VocoderAudioProcessorEditorImpl()
	{
	}
	virtual void paint(Graphics& g) override
	{
		g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
	}
	virtual void resized() override
	{
	}
};

VocoderAudioProcessorEditor* VocoderAudioProcessorEditor::createInstance(VocoderAudioProcessor& v)
{
	return new VocoderAudioProcessorEditorImpl(v);
}
