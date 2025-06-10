/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "ImageKnob.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "BinaryData.h"

static const char* paramIDs[4] = {"LowGain", "MidGain", "HighGain", "MidQ"};
static const char* paramLabels[4] = {"Low Gain", "Mid Gain", "High Gain", "Mid Q"};

class GainKnobLookAndFeels {
public:
    GainKnobLookAndFeels()
        : blackKnob(juce::ImageFileFormat::loadFrom(BinaryData::blackknob_png, BinaryData::blackknob_pngSize)),
          pinkKnob(juce::ImageFileFormat::loadFrom(BinaryData::pinkknob_png, BinaryData::pinkknob_pngSize)),
          whiteKnob(juce::ImageFileFormat::loadFrom(BinaryData::whiteknob_png, BinaryData::whiteknob_pngSize)) {}
    ImageKnobLookAndFeel blackKnob, pinkKnob, whiteKnob;
};

// Ajout d'une classe Timer interne pour la popup
class GainPopupTimer : public juce::Timer {
public:
    GainPopupTimer(std::function<void()> cb) : callback(std::move(cb)) {}
    void timerCallback() override { if (callback) callback(); }
private:
    std::function<void()> callback;
};

// LookAndFeel custom pour le bouton texte DIST
class DistTextButtonLookAndFeel : public juce::LookAndFeel_V4 {
public:
    DistTextButtonLookAndFeel() {
        distTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::Metropolitan_ttf, BinaryData::Metropolitan_ttfSize);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool, bool) override {
        // Fond transparent, contour noir
        auto bounds = button.getLocalBounds().toFloat();
        g.setColour(juce::Colours::transparentBlack);
        g.fillRect(bounds);
        g.setColour(juce::Colours::black);
        g.drawRect(bounds, 2.0f);
    }
    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool isMouseOverButton, bool) override {
        auto font = juce::Font(distTypeface.get()).withPointHeight(40.0f);
        g.setFont(font);
        auto onColour = juce::Colour::fromRGB(210, 122, 196); // MerjEQ
        auto offColour = juce::Colour::fromRGB(192, 192, 192); // Boomy
        juce::Colour baseColour = button.getToggleState() ? onColour : offColour;
        // Suppression de la transition hover : couleur stricte ON/OFF
        g.setColour(baseColour);
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
    }
private:
    juce::Typeface::Ptr distTypeface;
};

static DistTextButtonLookAndFeel distTextButtonLF;

namespace {
    std::unique_ptr<GainKnobLookAndFeels> gainKnobLFs;
}

MerjEQAudioProcessorEditor::MerjEQAudioProcessorEditor(MerjEQAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    // Fond
    auto* imgData = BinaryData::backgroundmodern_png;
    int imgSize = BinaryData::backgroundmodern_pngSize;
    auto loadedImg = juce::ImageFileFormat::loadFrom(imgData, imgSize);
    if (loadedImg.isValid())
        backgroundImage = std::make_unique<juce::Image>(loadedImg);
    else
        backgroundImage = nullptr;
    setSize(1152, 384);

    // Initialiser LookAndFeel pour les sliders de gain
    if (!gainKnobLFs)
        gainKnobLFs = std::make_unique<GainKnobLookAndFeels>();

    // Sliders et attachements
    for (int i = 0; i < 4; ++i)
    {
        sliders[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
        sliders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); // Supprime la boîte de valeur
        this->addAndMakeVisible(sliders[i]);
    }

    // Initialiser les sliders de gain à 0 dB (valeur centrale)
    sliders[0].setValue(0.0f);
    sliders[1].setValue(0.0f);
    sliders[2].setValue(0.0f);
    sliders[3].setValue(1.0f); // Q à 1 par défaut

    // Appliquer LookAndFeel personnalisé pour chaque slider de gain
    sliders[0].setLookAndFeel(&gainKnobLFs->blackKnob); // Low Gain
    sliders[1].setLookAndFeel(&gainKnobLFs->pinkKnob);  // Mid Gain
    sliders[2].setLookAndFeel(&gainKnobLFs->whiteKnob); // High Gain
    sliders[3].setLookAndFeel(&gainKnobLFs->pinkKnob);  // Mid Q (ImageKnob pink)
    gainKnobLFs->pinkKnob.setMidQSliderPointer(&sliders[3]);

    // Attachments
    for (int i = 0; i < 4; ++i)
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, paramIDs[i], sliders[i]);

    // Ajout des callbacks pour afficher la valeur du gain ou Q
    gainPopupSlider = -1;
    gainPopupText = "";
    gainPopupTimer.reset();

    for (int i = 0; i < 3; ++i) {
        sliders[i].onValueChange = [this, i]() {
            if (sliders[i].isMouseButtonDown()) {
                gainPopupText = juce::String(sliders[i].getValue(), 1) + " dB";
                gainPopupSlider = i;
                if (gainPopupTimer) gainPopupTimer->stopTimer();
                gainPopupTimer = std::make_unique<GainPopupTimer>([this]() { gainPopupSlider = -1; this->repaint(); });
                gainPopupTimer->startTimer(1500);
                this->repaint();
            }
        };
        sliders[i].onDragStart = [this, i]() {
            gainPopupText = juce::String(sliders[i].getValue(), 1) + " dB";
            gainPopupSlider = i;
            if (gainPopupTimer) gainPopupTimer->stopTimer();
            this->repaint();
        };
        sliders[i].onDragEnd = [this]() {
            if (gainPopupTimer) gainPopupTimer->stopTimer();
            gainPopupSlider = -1;
            this->repaint();
        };
    }

    sliders[3].onValueChange = [this]() {
        if (sliders[3].isMouseButtonDown()) {
            gainPopupText = juce::String(sliders[3].getValue(), 2);
            gainPopupSlider = 3;
            if (gainPopupTimer) gainPopupTimer->stopTimer();
            gainPopupTimer = std::make_unique<GainPopupTimer>([this]() { gainPopupSlider = -1; this->repaint(); });
            gainPopupTimer->startTimer(1500);
            this->repaint();
        }
    };
    sliders[3].onDragStart = [this]() {
        gainPopupText = juce::String(sliders[3].getValue(), 2);
        gainPopupSlider = 3;
        if (gainPopupTimer) gainPopupTimer->stopTimer();
        this->repaint();
    };
    sliders[3].onDragEnd = [this]() {
        if (gainPopupTimer) gainPopupTimer->stopTimer();
        gainPopupSlider = -1;
        this->repaint();
    };

    // --- Création du bouton de distorsion avec DrawableButton ---
    // Chargement des images avec Drawable pour préserver la transparence PNG
    distOnImage.reset(juce::Drawable::createFromImageData(BinaryData::SaturationON_png, 
                                                        BinaryData::SaturationON_pngSize).release());
    distOffImage.reset(juce::Drawable::createFromImageData(BinaryData::SaturationOFF_png, 
                                                        BinaryData::SaturationOFF_pngSize).release());
                                                        
    // Création du DrawableButton avec mode ImageOnButtonBackground
    distButton = std::make_unique<juce::DrawableButton>("DistButton", juce::DrawableButton::ImageOnButtonBackground);
    distButton->setImages(distOffImage.get(), nullptr, nullptr, nullptr,
                         distOnImage.get(), nullptr, nullptr, nullptr);
                         
    distButton->setClickingTogglesState(true);
    distButton->setToggleState(false, juce::dontSendNotification);
    distButton->onClick = [this]() {
        distState = distButton->getToggleState();
        repaint();
    };
    addAndMakeVisible(*distButton);

    // --- Ajout du bouton texte toggle DIST ---
    distTextButton = std::make_unique<juce::TextButton>("DIST");
    distTextButton->setBounds(50, 230, 180, 60); // 20px à gauche
    distTextButton->setButtonText("DIST");
    distTextButton->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    distTextButton->setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    distTextButton->setClickingTogglesState(true);
    distTextButton->setToggleState(false, juce::dontSendNotification);
    distTextButton->setLookAndFeel(&distTextButtonLF);
    distTextButton->onClick = [this]() {
        repaint();
    };
    addAndMakeVisible(*distTextButton);
}

MerjEQAudioProcessorEditor::~MerjEQAudioProcessorEditor()
{
    // Important: détacher LookAndFeel pour éviter les fuites
    for (int i = 0; i < 3; ++i)
        sliders[i].setLookAndFeel(nullptr);
}

void MerjEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Affiche le fond
    if (backgroundImage && backgroundImage->isValid())
        g.drawImage(*backgroundImage, getLocalBounds().toFloat());
    else
        g.fillAll(juce::Colours::black);

    // Affichage du titre du plugin en haut, centré, police Metropolitan.ttf, rose clair
    auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::Metropolitan_ttf, BinaryData::Metropolitan_ttfSize);
    juce::Font titleFont(typeface.get());
    titleFont = titleFont.withPointHeight(40.0f);
    g.setFont(titleFont);
    g.setColour(juce::Colour::fromRGB(210, 122, 196)); // rose clair
    g.drawFittedText("Merj EQ", 0, 10, getWidth(), 40, juce::Justification::centredTop, 1);

    // Titres personnalisés au-dessus de chaque knob
    juce::Font labelFont(typeface.get());
    labelFont = labelFont.withPointHeight(24.0f); // Taille initiale restaurée
    g.setFont(labelFont);
    g.setColour(juce::Colour::fromRGB(192, 192, 192)); // argenté
    // Low Gain
    g.drawFittedText("Boomy", sliders[0].getX(), sliders[0].getY() - 24, sliders[0].getWidth(), 20, juce::Justification::centred, 1);
    // Mid Gain
    g.drawFittedText("Clarity", sliders[1].getX(), sliders[1].getY() - 24, sliders[1].getWidth(), 20, juce::Justification::centred, 1);
    // Q
    g.drawFittedText("Q", sliders[3].getX(), sliders[3].getY() - 24, sliders[3].getWidth(), 20, juce::Justification::centred, 1);
    // High Gain
    g.drawFittedText("Brightness", sliders[2].getX(), sliders[2].getY() - 24, sliders[2].getWidth(), 20, juce::Justification::centred, 1);

    // --- Affichage du label "Dist" sous le bouton de distorsion ---
    if (distButton) {
        // Label "Dist" sous le bouton, police Metropolitan
        auto distTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::Metropolitan_ttf, BinaryData::Metropolitan_ttfSize);
        juce::Font distFont(distTypeface.get());
        distFont = distFont.withPointHeight(20.0f);
        g.setFont(distFont);
        g.setColour(juce::Colours::white);
        g.drawFittedText("Dist", distButton->getX(), distButton->getY() + distButton->getHeight() + 5,
                        distButton->getWidth(), 24, juce::Justification::centred, 1);
    }
}

void MerjEQAudioProcessorEditor::resized()
{
    // 3 sliders de gain à gauche, Mid Q à droite
    int gainKnobW = 180, gainKnobH = 220;
    int qKnobW = static_cast<int>(70 * 1.5f), qKnobH = static_cast<int>(90 * 1.5f); // Agrandit le knob Q de 0,5x
    int leftPad = 280; // décalage vers la droite (était 110, +100px)
    int topPad = 170; // abaissé de 10px (était 170)

    // Positionner le Low Gain (200Hz)
    sliders[0].setBounds(leftPad, topPad, gainKnobW, gainKnobH);
    // Positionner le Mid Gain (4kHz) plus proche du Low Gain
    int lowToMidMargin = 80; // serré entre 200Hz et 4kHz
    sliders[1].setBounds(leftPad + gainKnobW + lowToMidMargin, topPad, gainKnobW, gainKnobH);
    // Positionner le High Gain (12kHz) à sa position d'origine (espacement large)
    int midToHighMargin = 200; // espace large entre 4kHz et 12kHz
    sliders[2].setBounds(leftPad + 2 * gainKnobW + lowToMidMargin + midToHighMargin, topPad, gainKnobW, gainKnobH);

    // Placer le Mid Q entre le High Gain (12kHz, sliders[2]) et le Mid Gain (4kHz, sliders[1])
    int x1 = sliders[1].getX() + gainKnobW; // droite du Mid Gain
    int x2 = sliders[2].getX();             // gauche du High Gain
    int qX = x1 + (x2 - x1 - qKnobW) / 2 - 20;   // centré entre les deux, décalé 20px à gauche
    int qY = topPad + (gainKnobH - qKnobH) / 2; // aligné verticalement au centre des gros knobs
    sliders[3].setBounds(qX, qY, qKnobW, qKnobH);

    // Affiche la valeur du Mid Q en info-bulle (tooltip)
    sliders[3].setTooltip("Q: " + juce::String(sliders[3].getValue(), 2));

    // Positionnement du bouton de distorsion - coordonnées fixes
    if (distButton)
        distButton->setBounds(70, 230, 90, 90);

    if (distTextButton)
        distTextButton->setBounds(50, 140, 180, 60); // monté à y=140
}
