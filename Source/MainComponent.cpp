#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#define SMOOTHING 6     /* Smoothing factor used to determine 
                           color transition and sensitivity   */

class MainContentComponent   : public AudioAppComponent,
                               public MultiTimer
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
        startTimer(TimerIDs::COLOR, 1000/30);  // Color interpolation
        startTimer(TimerIDs::FRAME, 1000/60); // 60fps painting
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        last = rms;
        rms  = bufferToFill.buffer->getRMSLevel(0, bufferToFill.startSample, bufferToFill.numSamples);
    }

    void releaseResources() override
    {
        stopTimer(TimerIDs::COLOR);
        stopTimer(TimerIDs::FRAME);
        last = rms = 0;
    }

    void paint (Graphics& g) override
    {
        g.setColour(minColour.interpolatedWith(maxColour, currentColor));
        g.fillAll();
    }

    void resized() override
    {

    }

    void timerCallback(int timerID) override
    {
        switch (timerID)
        {
            case TimerIDs::COLOR:
                
                if (std::abs(rms - last) > 0.001)
                {
                    targetColor = expm1f((rms + last) / 2) * SMOOTHING;
                }
                
                break;
                
            case TimerIDs::FRAME:
                
                if (currentColor < targetColor)
                {
                    currentColor += std::abs(currentColor - targetColor) / SMOOTHING;
                }
                else
                {
                    currentColor -= std::abs(currentColor - targetColor) / SMOOTHING;
                }
                
                if (std::abs(rms - last) > 0.001)
                {
                    repaint();
                }
                
                break;
                
            default:
                break;
        }
    }

private:
    float rms;
    float last;
    
    float targetColor;
    float currentColor;
    
    enum TimerIDs
    {
        FRAME,
        COLOR
    };
    
    Colour minColour = Colours::blue;
    Colour maxColour = Colours::red;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_H_INCLUDED
