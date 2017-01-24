#ifndef MAINCOMPONENT_INCLUDED
#define MAINCOMPONENT_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

struct MainContentComponent   : public AudioAppComponent,
                                public Timer,
                                public Slider::Listener,
                                public Button::Listener
{
    MainContentComponent()
    {
        baseSensitivity = 0;
        sensitivity     = 12;
        smoothing       = 6;

        addAndMakeVisible(settingsPanel);

        colourPanel.addMouseListener(this, false);
        addAndMakeVisible(colourPanel);
        
        settingsButton.addListener(this);
        colourPanel.addAndMakeVisible(settingsButton);
        
        settingsPanel.sensitivitySlider.setValue(sensitivity);
        settingsPanel.sensitivitySlider.addListener(this);
        
        settingsPanel.smoothingSlider.setValue(smoothing);
        settingsPanel.smoothingSlider.addListener(this);

        settingsPanel.calibrateButton.addListener(this);
        
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
        startTimer(1000/30);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        lastrms = rms;
        rms  = bufferToFill.buffer->getRMSLevel(0, bufferToFill.startSample, bufferToFill.numSamples);
    }

    void releaseResources() override
    {
        stopTimer();
        lastrms = rms = 0;
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
        
        settingsPanel.setBounds(0, 0, (w * 2) / 3, h);
        settingsButton.setBounds(w/23, w/23, (h/18) / 1.2, h/18);
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
        if (b == &settingsButton)
        {
            settingsOpen = (settingsOpen) ? false : true;
            triggerAnimation();
        }

        if (b == &settingsPanel.calibrateButton)
        {

            if (!hasBeenCalibrated)
            {

                if (!calibrating)
                {
                    calibrating = true;
                    settingsPanel.calibrateButton.setButtonText("Stop Calibration");
                    settingsPanel.calibrateButton.setColour(TextButton::ColourIds::buttonColourId, Colours::palevioletred);
                }
                else
                {
                    calibrating = false;
                    hasBeenCalibrated = true;
                    settingsPanel.calibrateButton.setButtonText("Reset Calibration");
                    settingsPanel.calibrateButton.setColour(TextButton::ColourIds::buttonColourId, Colours::grey);
                }
            }
            else
            {
                calibrating       = false;
                baseSensitivity   = 0;
                hasBeenCalibrated = false;
                settingsPanel.calibrateButton.setButtonText("Calibrate");
                settingsPanel.calibrateButton.setColour(TextButton::ColourIds::buttonColourId, Colours::palegreen);
            }
        }
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

    void timerCallback() override
    {
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
        
        if (std::abs(rms - lastrms) > 0.001)
        {
            targetColour = expm1f((rms + lastrms) / 2) * ((sensitivity + baseSensitivity)/smoothing);
        }
        
        if (currentColour < targetColour)
        {
            currentColour += std::abs(currentColour - targetColour) / smoothing;
        }
        else
        {
            currentColour -= std::abs(currentColour - targetColour) / smoothing;
        }
        
        if (std::abs(rms - lastrms) > 0.001)
        {
            colourPanel.displayColour = Colour(minColour.interpolatedWith(maxColour, currentColour));
            colourPanel.repaint();
        }

        if (calibrating)
        {
            if (targetColour < 1)
            {
                baseSensitivity += 0.25;
            }
            else
            {
                baseSensitivity -= 0.25;
            }
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
                g.fillRoundedRectangle(0, ((h/3) - 1) * i, w, h/6, h/12);
        }
    };
    
    struct SettingsPanel : public Component,
                           public LookAndFeel_V3
    {
        SettingsPanel()
        {
            setDefaultLookAndFeel(this);

            sensitivitySlider.setRange(1, 18);
            sensitivitySlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
            addAndMakeVisible(sensitivitySlider);
            
            sensitivityLabel.setText("Sensitivity", dontSendNotification);
            sensitivityLabel.attachToComponent(&sensitivitySlider, false);
            addAndMakeVisible(sensitivityLabel);
            
            smoothingSlider.setRange(1, 12);
            smoothingSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
            addAndMakeVisible(smoothingSlider);
            
            smoothingLabel.setText("Smoothing", dontSendNotification);
            smoothingLabel.attachToComponent(&smoothingSlider, false);
            addAndMakeVisible(smoothingLabel);

            calibrateButton.setButtonText("Calibrate");
            calibrateButton.setColour(TextButton::ColourIds::buttonColourId, Colours::palegreen);
            addAndMakeVisible(calibrateButton);
        }
        
        void paint(Graphics& g) override
        {
            int w = getWidth();
            int h = getHeight();
            
            g.fillAll(Colours::darkgrey);
            
            g.setColour(Colours::lightgrey);
            
            g.setFont(h/24);
            g.drawFittedText(
                "Settings",
                0,
                10,
                w,
                h/24,
                Justification::centred,
                1
            );
        }
        
        void resized() override
        {
            int w = getWidth();
            int h = getHeight();
            
            for (int i = 0; i < getNumChildComponents(); ++i)
                if (dynamic_cast<Slider*>(getChildComponent(i)) || dynamic_cast<TextButton*>(getChildComponent(i)))
                    getChildComponent(i)->setBounds(10, ((h/16) * i) + h/8, w - 20, h/16);
        }
        
        void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle, Slider& s) override
        {
            width  = s.getWidth();
            height = s.getHeight();
            sliderPos = (s.getValue() == 1) ? 0 : s.getValue()/s.getMaximum();
            
            g.setColour(Colours::grey);
            g.fillRoundedRectangle(0, (height/2) - (height/8), width, height/4, height/8);
            
            g.setColour(Colours::lightgrey);
            g.fillRoundedRectangle(0, (height/2) - (height/8), sliderPos * (width), height/4, height/8);
            g.fillEllipse(sliderPos * (width - height/2), (height/2) - (height/4), height/2, height/2);
        }
        
        void drawLabel(Graphics& g, Label& l) override
        {
            g.setColour(Colours::lightgrey);
            g.drawFittedText(l.getText(), 0, 0, l.getWidth(), l.getHeight(), Justification::centredLeft, 1);
        }

        void drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override
        {
            int w = b.getWidth();
            int h = b.getHeight();
            g.setColour(backgroundColour);
            g.fillRoundedRectangle(0, 0, w, h, 10);
        }

        void drawButtonText(Graphics& g, TextButton& b, bool isMouseOverButton, bool isButtonDown) override
        {
            int w = b.getWidth();
            int h = b.getHeight();
            String s = b.getButtonText();

            g.setColour(Colours::darkgrey);
            g.drawFittedText(s, 0, 0, w, h, Justification::centred, 1);
        }

        Slider sensitivitySlider;
        Label sensitivityLabel;
        
        Slider smoothingSlider;
        Label smoothingLabel;

        TextButton calibrateButton;
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
    
    float rms, lastrms,
          targetColour, currentColour,
          baseSensitivity, sensitivity,
          smoothing;

    bool settingsOpen      = false;
    bool calibrating       = false;
    bool hasBeenCalibrated = false;
    
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
