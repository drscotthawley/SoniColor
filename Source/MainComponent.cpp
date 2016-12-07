#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#define FrameTimer 0
#define ColorTimer 1

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
        startTimer(ColorTimer, 1000/30);  // Color interpolation
        startTimer(FrameTimer, 1000/60); // 60fps painting
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        last = rms;
        rms  = bufferToFill.buffer->getRMSLevel(0, bufferToFill.startSample, bufferToFill.numSamples);
    }

    void releaseResources() override
    {
        stopTimer(ColorTimer);
        stopTimer(FrameTimer);
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
            case ColorTimer:
                
                if (std::abs(rms - last) > 0.001)
                {
                    targetColor = expm1f((rms + last) / 2) * (sensitivity/smoothing);
                }
                
                break;
                
            case FrameTimer:
                
                if (currentColor < targetColor)
                {
                    currentColor += std::abs(currentColor - targetColor) / smoothing;
                }
                else
                {
                    currentColor -= std::abs(currentColor - targetColor) / smoothing;
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
    
    uint8 sensitivity = 12, smoothing = 6;
    
    Colour minColour = Colour(74, 168, 219);
    Colour maxColour = Colour(227, 109, 80);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_H_INCLUDED
