#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent   : public AudioAppComponent,
                               public Timer
{
public:
    MainContentComponent()
    {
        setSize (800, 600);
        setAudioChannels (1, 0);
    }

    ~MainContentComponent()
    {
        shutdownAudio();
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        startTimerHz(60);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        currentRMS[1] = currentRMS[0];
        currentRMS[0] = bufferToFill.buffer->getRMSLevel(0, bufferToFill.startSample, bufferToFill.numSamples);
    }

    void releaseResources() override
    {
        stopTimer();
    }

    void paint (Graphics& g) override
    {
        g.setColour(minColour.interpolatedWith(maxColour, currentRMS[0]));
        g.fillAll();
    }

    void resized() override
    {

    }

    void timerCallback() override
    {
        repaint();
    }

private:
    float currentRMS[2];
    
    Colour minColour = Colours::blue;
    Colour maxColour = Colours::red;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_H_INCLUDED
