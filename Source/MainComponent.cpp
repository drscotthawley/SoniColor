#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#define SMOOTHING 4

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
        frame = 0;
        avg.resize(SMOOTHING);
        startTimerHz(60);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        rms = bufferToFill.buffer->getRMSLevel(0, bufferToFill.startSample, bufferToFill.numSamples);
    }

    void releaseResources() override
    {
        stopTimer();
        avg.clear();
    }

    void paint (Graphics& g) override
    {
        g.setColour(minColour.interpolatedWith(maxColour, intrp));
        g.fillAll();
    }

    void resized() override
    {

    }

    void timerCallback() override
    {
        frame = (frame <= SMOOTHING) ? frame + 1 : 0;
        
        avg[frame] = rms;
        
        float sum = 0;
        
        for (std::vector<float>::iterator i = avg.begin(); i < avg.end(); ++i)
        {
            sum += *i;
        }
        
        intrp = sum / avg.size();
        
        repaint();
    }

private:
    int frame;
    float rms;
    float intrp;
    std::vector<float> avg;
    
    Colour minColour = Colours::blue;
    Colour maxColour = Colours::red;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_H_INCLUDED
