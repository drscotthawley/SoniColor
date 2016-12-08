#ifndef MAINCOMPONENT_INCLUDED
#define MAINCOMPONENT_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#define FrameTimer 0
#define ColorTimer 1

struct MainContentComponent   : public AudioAppComponent,
                                public MultiTimer,
                                public Slider::Listener,
                                public Button::Listener
{
    MainContentComponent()
    {
        sensitivity = 12;
        smoothing = 6;

        addAndMakeVisible(settingsPanel);

        colourPanel.addMouseListener(this, false);
        addAndMakeVisible(colourPanel);
        
        settingsButton.addListener(this);
        colourPanel.addAndMakeVisible(settingsButton);
        
        settingsPanel.sensitivitySlider.setValue(sensitivity);
        settingsPanel.sensitivitySlider.addListener(this);
        
        settingsPanel.smoothingSlider.setValue(smoothing);
        settingsPanel.smoothingSlider.addListener(this);
        
        screenSize = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
        setSize (screenSize.getWidth(), screenSize.getHeight());
        
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

    void mouseDown(const MouseEvent& e) override
    {
        if (settingsOpen)
        {
            settingsOpen = false;
            triggerAnimation();
        }
    }
    
    void paint (Graphics& g) override
    {
        g.fillAll(Colours::black);
    }

    void resized() override
    {
        int w = screenSize.getWidth();
        int h = screenSize.getHeight();
        
        settingsPanel.setBounds(0, 0, (w * 2)/3, h);
        settingsButton.setBounds(w/23, w/23, (h/18)/1.2, h/18);
        colourPanel.setBounds(0, 0, w, h);
        
        animationStartPoint = colourPanel.getBounds();
        animationEndPoint   = colourPanel.getBounds().translated((w * 2)/3, 0);
    }
    
    void sliderValueChanged(Slider* s) override
    {
        if (s == &settingsPanel.sensitivitySlider)
        {
            sensitivity = s->getValue();
        }
        else if (s == &settingsPanel.smoothingSlider)
        {
            smoothing = s->getValue();
        }
    }
    
    void buttonClicked(Button* b) override
    {
        settingsOpen = (settingsOpen) ? false : true;
        triggerAnimation();
    }
    
    void triggerAnimation()
    {
        if (settingsOpen)
        {
            animator.animateComponent(&colourPanel, animationEndPoint, 1.0, 200, false, 0, 1);
        }
        else
        {
            animator.animateComponent(&colourPanel, animationStartPoint, 1.0, 200, false, 0, 1);
        }
    }

    void timerCallback(int timerID) override
    {
        switch (timerID)
        {
            case ColorTimer:
                
                if (std::abs(rms - last) > 0.001)
                {
                    targetColour = expm1f((rms + last) / 2) * (sensitivity/smoothing);
                }
                
                break;
                
            case FrameTimer:
                
                if (!animator.isAnimating() && !settingsOpen)
                {
                    settingsPanel.setVisible(false);
                }
                else
                {
                    if (!settingsPanel.isVisible())
                    {
                        settingsPanel.setVisible(true);
                        settingsPanel.toBack();
                    }
                }
                
                if (currentColour < targetColour)
                {
                    currentColour += std::abs(currentColour - targetColour) / smoothing;
                }
                else
                {
                    currentColour -= std::abs(currentColour - targetColour) / smoothing;
                }
                
                if (std::abs(rms - last) > 0.001)
                {
                    colourPanel.displayColour = Colour(minColour.interpolatedWith(maxColour, currentColour));
                    colourPanel.repaint();
                }
                
                break;
                
            default:
                break;
        }
    }
    
    struct SettingsButton : public Button
    {
        SettingsButton() : Button("Settings") {};
        
        void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            int w = getWidth();
            int h = getHeight();

            g.setColour(Colours::white.withAlpha((uint8) 64));
            
            for (int i = 0; i < 3; i++)
                g.fillRoundedRectangle(0, ((h/3)-1) * i, w, h/6, h/12);
        }
    };
    
    struct SettingsPanel : public Component
    {
        SettingsPanel()
        {
            sensitivitySlider.setRange(1, 18);
            sensitivitySlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
            addAndMakeVisible(sensitivitySlider);
            
            smoothingSlider.setRange(1, 12);
            smoothingSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
            addAndMakeVisible(smoothingSlider);
        }
        
        void paint(Graphics& g) override
        {
            int w = getWidth();
            int h = getHeight();
            
            g.fillAll(Colours::darkgrey);
            
            g.setColour(Colours::grey);
            
            g.setFont(h/24);
            g.drawFittedText("Settings", 0, 10, w, h/24, Justification::centred, 1);
            
            
            g.setFont(h/48);
            g.drawFittedText(
                "Sensitivity",
                sensitivitySlider.getX(),
                sensitivitySlider.getY(),
                sensitivitySlider.getWidth(),
                h/48,
                Justification::centred,
                1
            );
            
            g.drawFittedText(
                "Smoothing",
                smoothingSlider.getX(),
                smoothingSlider.getY(),
                smoothingSlider.getWidth(),
                h/48,
                Justification::centred,
                1
            );
        }
        
        void resized() override
        {
            int w = getWidth();
            int h = getHeight();
            
            std::vector<Component*> components = {
                &sensitivitySlider,
                &smoothingSlider
            };
            
            for (int i = 0; i < components.size(); ++i)
                components[i]->setBounds(10, ((h/8) * i) + (i * h/16) + h/8, w - 20, h/8);
        }
        
        Slider sensitivitySlider;
        Slider smoothingSlider;
    };
    
    struct ColourPanel : public Component
    {
        void paint(Graphics& g) override
        {
            g.setColour(displayColour);
            g.fillAll();
        }
        
        Colour displayColour = Colours::black;
    };
    
    float rms, last,
          targetColour, currentColour,
          sensitivity, smoothing;
    
    bool settingsOpen = false;
    
    Colour minColour = Colour(74, 168, 219);
    Colour maxColour = Colour(227, 109, 80);
    
    SettingsButton settingsButton;
    SettingsPanel  settingsPanel;
    ColourPanel    colourPanel;

    ComponentAnimator animator;
    Rectangle<int>    animationStartPoint;
    Rectangle<int>    animationEndPoint;
    
    Rectangle<int> screenSize;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_INCLUDED
