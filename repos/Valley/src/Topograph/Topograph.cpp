//
// Topograph.cpp
// Author: Dale Johnson
// Contact: valley.audio.soft@gmail.com
// Date: 5/12/2017
//
// Topograph, a port of "Mutable Instruments Grids" for VCV Rack
// Original author: Olivier Gillet (ol.gillet@gmail.com)
// https://github.com/pichenettes/eurorack/tree/master/grids
// Copyright 2012 Olivier Gillet.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "../Valley.hpp"
#include "dsp/digital.hpp"
#include "../Common/Metronome.hpp"
#include "../Common/Oneshot.hpp"
#include "TopographPatternGenerator.hpp"
#include "DynamicBase.hpp"

struct Topograph : Module {
    enum ParamIds {
        RESET_BUTTON_PARAM,
        RUN_BUTTON_PARAM,
        TEMPO_PARAM,
        MAPX_PARAM,
        MAPY_PARAM,
        CHAOS_PARAM,
        BD_DENS_PARAM,
        SN_DENS_PARAM,
        HH_DENS_PARAM,
        SWING_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_INPUT,
        RESET_INPUT,
        MAPX_CV,
        MAPY_CV,
        CHAOS_CV,
        BD_FILL_CV,
        SN_FILL_CV,
        HH_FILL_CV,
        SWING_CV,
        RUN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        BD_OUTPUT,
        SN_OUTPUT,
        HH_OUTPUT,
        BD_ACC_OUTPUT,
        SN_ACC_OUTPUT,
        HH_ACC_OUTPUT,
        NUM_OUTPUTS
    };

    enum LightIds {
        RUNNING_LIGHT,
        RESET_LIGHT,
        BD_LIGHT,
        SN_LIGHT,
        HH_LIGHT,
        NUM_LIGHTS
    };

    Metronome metro;
    PatternGenerator grids;
    uint8_t numTicks;
    SchmittTrigger clockTrig;
    SchmittTrigger resetTrig;
    SchmittTrigger resetButtonTrig;
    SchmittTrigger runButtonTrig;
    SchmittTrigger runInputTrig;
    bool initExtReset = true;
    int running = 0;
    bool extClock = false;
    bool advStep = false;
    long seqStep = 0;
    float swing = 0.5;
    float swingHighTempo = 0.0;
    float swingLowTempo = 0.0;
    long elapsedTicks = 0;

    float tempoParam = 0.0;
    float tempo = 120.0;
    float mapX = 0.0;
    float mapY = 0.0;
    float chaos = 0.0;
    float BDFill = 0.0;
    float SNFill = 0.0;
    float HHFill = 0.0;

    uint8_t state = 0;

    // LED Triggers
    Oneshot drumLED[3];
    const LightIds drumLEDIds[3] = {BD_LIGHT, SN_LIGHT, HH_LIGHT};
    Oneshot BDLed;
    Oneshot SNLed;
    Oneshot HHLed;
    Oneshot resetLed;
    Oneshot runningLed;

    // Drum Triggers
    Oneshot drumTriggers[6];
    bool gateState[6];
    const OutputIds outIDs[6] = {BD_OUTPUT, SN_OUTPUT, HH_OUTPUT,
                                 BD_ACC_OUTPUT, SN_ACC_OUTPUT, HH_ACC_OUTPUT};

    enum SequencerMode {
        HENRI,
        OLIVIER,
        EUCLIDEAN
    };
    SequencerMode sequencerMode = HENRI;

    enum TriggerOutputMode {
        PULSE,
        GATE
    };
    TriggerOutputMode triggerOutputMode = PULSE;

    enum AccOutputMode {
        INDIVIDUAL_ACCENTS,
        ACC_CLK_RST
    };
    AccOutputMode accOutputMode = INDIVIDUAL_ACCENTS;

    enum ExtClockResolution {
        EXTCLOCK_RES_4_PPQN,
        EXTCLOCK_RES_8_PPQN,
        EXTCLOCK_RES_24_PPQN,
    };
    ExtClockResolution extClockResolution = EXTCLOCK_RES_24_PPQN;

    enum ChaosKnobMode {
        CHAOS,
        SWING
    };
    ChaosKnobMode chaosKnobMode = CHAOS;

    enum RunMode {
        TOGGLE,
        MOMENTARY
    };
    RunMode runMode = TOGGLE;

    int panelStyle;
    std::string clockBPM;
    std::string mapXText = "Map X";
    std::string mapYText = "Map Y";
    std::string chaosText = "Chaos";
    int textVisible = 1;

    Topograph() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
        metro = Metronome(120, engineGetSampleRate(), 24.0, 0.0);
        numTicks = ticks_granularity[2];
        srand(time(NULL));
        BDLed = Oneshot(0.1, engineGetSampleRate());
        SNLed = Oneshot(0.1, engineGetSampleRate());
        HHLed = Oneshot(0.1, engineGetSampleRate());
        resetLed = Oneshot(0.1, engineGetSampleRate());
        //clockTrig.setThresholds(0.25, 0.75);
        //resetTrig.setThresholds(0.25, 0.75);
        //runInputTrig.setThresholds(0.25, 0.75);
        for(int i = 0; i < 6; ++i) {
            drumTriggers[i] = Oneshot(0.001, engineGetSampleRate());
            gateState[i] = false;
        }
        for(int i = 0; i < 3; ++i) {
            drumLED[i] = Oneshot(0.1, engineGetSampleRate());
        }
        panelStyle = 0;
    }

    json_t *toJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "sequencerMode", json_integer(sequencerMode));
        json_object_set_new(rootJ, "triggerOutputMode", json_integer(triggerOutputMode));
        json_object_set_new(rootJ, "accOutputMode", json_integer(accOutputMode));
        json_object_set_new(rootJ, "extClockResolution", json_integer(extClockResolution));
        json_object_set_new(rootJ, "chaosKnobMode", json_integer(chaosKnobMode));
        json_object_set_new(rootJ, "runMode", json_integer(runMode));
        json_object_set_new(rootJ, "panelStyle", json_integer(panelStyle));
        return rootJ;
    }

    void fromJson(json_t *rootJ) override {
        json_t *sequencerModeJ = json_object_get(rootJ, "sequencerMode");
        if (sequencerModeJ) {
            sequencerMode = (Topograph::SequencerMode) json_integer_value(sequencerModeJ);
            switch(sequencerMode) {
                case HENRI:
                    grids.setPatternMode(PATTERN_HENRI);
                    break;
                case OLIVIER:
                    grids.setPatternMode(PATTERN_OLIVIER);
                    break;
                case EUCLIDEAN:
                    grids.setPatternMode(PATTERN_EUCLIDEAN);
                    break;
            }
		}

        json_t *triggerOutputModeJ = json_object_get(rootJ, "triggerOutputMode");
		if (triggerOutputModeJ) {
			triggerOutputMode = (Topograph::TriggerOutputMode) json_integer_value(triggerOutputModeJ);
		}

        json_t *accOutputModeJ = json_object_get(rootJ, "accOutputMode");
		if (accOutputModeJ) {
			accOutputMode = (Topograph::AccOutputMode) json_integer_value(accOutputModeJ);
            switch(accOutputMode) {
                case INDIVIDUAL_ACCENTS:
                    grids.setAccentAltMode(false);
                    break;
                case ACC_CLK_RST:
                    grids.setAccentAltMode(true);
            }
		}

        json_t *extClockResolutionJ = json_object_get(rootJ, "extClockResolution");
		if (extClockResolutionJ) {
			extClockResolution = (Topograph::ExtClockResolution) json_integer_value(extClockResolutionJ);
            grids.reset();
		}

        json_t *chaosKnobModeJ = json_object_get(rootJ, "chaosKnobMode");
		if (chaosKnobModeJ) {
			chaosKnobMode = (Topograph::ChaosKnobMode) json_integer_value(chaosKnobModeJ);
		}

        json_t *runModeJ = json_object_get(rootJ, "runMode");
		if (runModeJ) {
			runMode = (Topograph::RunMode) json_integer_value(runModeJ);
		}

        json_t *panelStyleJ = json_object_get(rootJ, "panelStyle");
        if (panelStyleJ) {
            panelStyle = (int)json_integer_value(panelStyleJ);
        }
	}

    void step() override;
    void onSampleRateChange() override;
    void updateUI();
    void updateOutputs();
};

void Topograph::step() {
    if(runMode == TOGGLE) {
        if (runButtonTrig.process(params[RUN_BUTTON_PARAM].value) ||
            runInputTrig.process(inputs[RUN_INPUT].value)) {
            if(runMode == TOGGLE){
                running = !running;
                lights[RUNNING_LIGHT].value = running ? 1.0 : 0.0;
            }
        }
    }
    else {
        running = params[RUN_BUTTON_PARAM].value + inputs[RUN_INPUT].value;
        lights[RUNNING_LIGHT].value = running ? 1.0 : 0.0;
        if(running == 0) {
            metro.reset();
        }
    }

    if(resetButtonTrig.process(params[RESET_BUTTON_PARAM].value) ||
        resetTrig.process(inputs[RESET_INPUT].value)) {
        grids.reset();
        metro.reset();
        resetLed.trigger();
        seqStep = 0;
        elapsedTicks = 0;
    }

    // Clock, tempo and swing
    tempoParam = params[TEMPO_PARAM].value;
    tempo = rescale(tempoParam, 0.01f, 1.f, 40.f, 240.f);
    char clockBPMChar[16];
    sprintf(clockBPMChar, "%.1f", tempo);
    clockBPM = clockBPMChar;
    swing = clamp(params[SWING_PARAM].value + inputs[SWING_CV].value / 10.f, 0.f, 0.9f);
    swingHighTempo = tempo / (1 - swing);
    swingLowTempo = tempo / (1 + swing);
    if(elapsedTicks < 6) {
        metro.setTempo(swingLowTempo);
    }
    else {
        metro.setTempo(swingHighTempo);
    }

    // External clock select
    if(tempoParam < 0.01) {
        clockBPM = "Ext.";
        if(initExtReset) {
            grids.reset();
            initExtReset = false;
        }
        numTicks = ticks_granularity[extClockResolution];
        extClock = true;
    }
    else {
        initExtReset = true;
        numTicks = ticks_granularity[2];
        extClock = false;
        metro.process();
    }

    mapX = params[MAPX_PARAM].value + (inputs[MAPX_CV].value / 10.f);
    mapX = clamp(mapX, 0.f, 1.f);
    mapY = params[MAPY_PARAM].value + (inputs[MAPY_CV].value / 10.f);
    mapY = clamp(mapY, 0.f, 1.f);
    BDFill = params[BD_DENS_PARAM].value + (inputs[BD_FILL_CV].value / 10.f);
    BDFill = clamp(BDFill, 0.f, 1.f);
    SNFill = params[SN_DENS_PARAM].value + (inputs[SN_FILL_CV].value / 10.f);
    SNFill = clamp(SNFill, 0.f, 1.f);
    HHFill = params[HH_DENS_PARAM].value + (inputs[HH_FILL_CV].value / 10.f);
    HHFill = clamp(HHFill, 0.f, 1.f);
    chaos = params[CHAOS_PARAM].value + (inputs[CHAOS_CV].value / 10.f);
    chaos = clamp(chaos, 0.f, 1.f);
    if(grids.getPatternMode() == PATTERN_EUCLIDEAN) {
        mapXText = "1 Len: " + std::to_string(((uint8_t)(mapX * 255.0) >> 3) + 1);
        mapYText = "2 Len: " + std::to_string(((uint8_t)(mapY * 255.0) >> 3) + 1);
        chaosText = "3 Len: " + std::to_string(((uint8_t)(chaos * 255.0) >> 3) + 1);
    }
    else {
        mapXText = "Map X";
        mapYText = "Map Y";
        chaosText = "Chaos";
    }

    if(running) {
        if(extClock) {
            if(clockTrig.process(inputs[CLOCK_INPUT].value)) {
                advStep = true;
            }
        }
        else if(metro.hasTicked()){
            advStep = true;
            elapsedTicks++;
            elapsedTicks %= 12;
        }
        else {
            advStep = false;
        }

        grids.setMapX((uint8_t)(mapX * 255.0));
        grids.setMapY((uint8_t)(mapY * 255.0));
        grids.setBDDensity((uint8_t)(BDFill * 255.0));
        grids.setSDDensity((uint8_t)(SNFill * 255.0));
        grids.setHHDensity((uint8_t)(HHFill * 255.0));
        grids.setRandomness((uint8_t)(chaos * 255.0));

        grids.setEuclideanLength(0, (uint8_t)(mapX * 255.0));
        grids.setEuclideanLength(1, (uint8_t)(mapY * 255.0));
        grids.setEuclideanLength(2, (uint8_t)(chaos * 255.0));
    }

    if(advStep) {
        grids.tick(numTicks);
        for(int i = 0; i < 6; ++i) {
            if(grids.getDrumState(i)) {
                drumTriggers[i].trigger();
                gateState[i] = true;
                if(i < 3) {
                    drumLED[i].trigger();
                }
            }
        }
        seqStep++;
        if(seqStep >= 32) {
            seqStep = 0;
        }
        advStep = false;
    }
    updateOutputs();
    updateUI();
}

void Topograph::updateUI() {

    resetLed.process();
    for(int i = 0; i < 3; ++i) {
        drumLED[i].process();
        if(drumLED[i].getState() == 1) {
            lights[drumLEDIds[i]].value = 1.0;
        }
        else {
            lights[drumLEDIds[i]].value = 0.0;
        }
    }


    if(resetLed.getState() == 1) {
        lights[RESET_LIGHT].value = 1.0;
    }
    else {
        lights[RESET_LIGHT].value = 0.0;
    }
}

void Topograph::updateOutputs() {
    if(triggerOutputMode == PULSE) {
        for(int i = 0; i < 6; ++i) {
            drumTriggers[i].process();
            if(drumTriggers[i].getState()) {
                outputs[outIDs[i]].value = 10;
            }
            else {
                outputs[outIDs[i]].value = 0;
            }
        }
    }
    else {
        for(int i = 0; i < 6; ++i) {
            if(metro.getElapsedTickTime() < 0.5 && gateState[i]) {
                outputs[outIDs[i]].value = 10;
            }
            else {
                outputs[outIDs[i]].value = 0;
                gateState[i] = false;
            }
        }
    }
}

void Topograph::onSampleRateChange() {
    metro.setSampleRate(engineGetSampleRate());
    for(int i = 0; i < 3; ++i) {
        drumLED[i].setSampleRate(engineGetSampleRate());
    }
    resetLed.setSampleRate(engineGetSampleRate());
    for(int i = 0; i < 6; ++i) {
        drumTriggers[i].setSampleRate(engineGetSampleRate());
    }
}

// The widget

struct PanelBorder : TransparentWidget {
	void draw(NVGcontext *vg) override {
		NVGcolor borderColor = nvgRGBAf(0.5, 0.5, 0.5, 0.5);
		nvgBeginPath(vg);
		nvgRect(vg, 0.5, 0.5, box.size.x - 1.0, box.size.y - 1.0);
		nvgStrokeColor(vg, borderColor);
		nvgStrokeWidth(vg, 1.0);
		nvgStroke(vg);
	}
};

struct DynamicPanel : FramebufferWidget {
    int* mode;
    int oldMode;
    std::vector<std::shared_ptr<SVG>> panels;
    SVGWidget* panel;

    DynamicPanel() {
        mode = nullptr;
        oldMode = -1;
        panel = new SVGWidget();
        addPanel(SVG::load(assetPlugin(plugin, "res/TopographPanel.svg")));
        addPanel(SVG::load(assetPlugin(plugin, "res/TopographPanelWhite.svg")));
        addChild(panel);

        PanelBorder *pb = new PanelBorder();
        pb->box.size = box.size;
        addChild(pb);
    }

    void addPanel(std::shared_ptr<SVG> svg) {
        panels.push_back(svg);
        if(!panel->svg) {
            panel->setSVG(svg);
            box.size = panel->box.size;
        }
    }

    void step() override {
        if(mode != nullptr) {
            panel->setSVG(panels[*mode]);
            dirty = true;
        }
    }
};

struct DynamicText : TransparentWidget {
    std::string oldText;
    std::string* pText;
    std::shared_ptr<Font> font;
    int size;
    NVGcolor drawColour;
    int* visibility;
    DynamicViewMode viewMode;

    enum Colour {
        COLOUR_WHITE,
        COLOUR_BLACK
    };
    int* colourHandle;

    DynamicText() {
        font = Font::load(assetPlugin(plugin, "res/din1451alt.ttf"));
        size = 16;
        visibility = nullptr;
        pText = nullptr;
        viewMode = ACTIVE_HIGH;
    }

    void draw(NVGcontext* vg) {
        nvgFontSize(vg, size);
        nvgFontFaceId(vg, font->handle);
        nvgTextLetterSpacing(vg, 0.f);
        Vec textPos = Vec(0.f, 0.f);
        if(colourHandle != nullptr) {
            switch(*colourHandle) {
                case COLOUR_BLACK : drawColour = nvgRGB(0x00,0x00,0x00); break;
                case COLOUR_WHITE : drawColour = nvgRGB(0xFF,0xFF,0xFF); break;
                default : drawColour = nvgRGB(0x00,0x00,0x00);
            }
        }
        else {
            drawColour = nvgRGB(0x00,0x00,0x00);
        }

        nvgFillColor(vg, drawColour);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        if(pText != nullptr) {
            nvgText(vg, textPos.x, textPos.y, pText->c_str(), NULL);
        }
    }

    void step() {
        if(visibility != nullptr) {
            if(*visibility) {
                visible = true;
            }
            else {
                visible = false;
            }
            if(viewMode == ACTIVE_LOW) {
                visible = !visible;
            }
        }
        else {
            visible = true;
        }
    }
};

DynamicText* createDynamicText(const Vec& pos, int size, int* colourHandle, std::string* pText,
                               int* visibilityHandle, DynamicViewMode viewMode) {
    DynamicText* dynText = new DynamicText();
    dynText->size = size;
    dynText->colourHandle = colourHandle;
    dynText->pText = pText;
    dynText->box.pos = pos;
    dynText->box.size = Vec(82,14);
    dynText->visibility = visibilityHandle;
    dynText->viewMode = viewMode;
    return dynText;
}

struct Rogan1PSBrightRed : Rogan {
    Rogan1PSBrightRed() {
        setSVG(SVG::load(assetPlugin(plugin, "res/Rogan1PSBrightRed.svg")));
    }
};

struct Rogan1PSOrange : Rogan {
    Rogan1PSOrange() {
        setSVG(SVG::load(assetPlugin(plugin, "res/Rogan1PSOrange.svg")));
    }
};

struct Rogan1PSYellow : Rogan {
    Rogan1PSYellow() {
        setSVG(SVG::load(assetPlugin(plugin, "res/Rogan1PSYellow.svg")));
    }
};

struct LightLEDButton : SVGSwitch, MomentarySwitch {
    LightLEDButton() {
        addFrame(SVG::load(assetPlugin(plugin, "res/LightLEDButton.svg")));
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Context Menu ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

struct TopographWidget : ModuleWidget {
    TopographWidget(Topograph *topograph);
    void appendContextMenu(Menu* menu) override;
};

TopographWidget::TopographWidget(Topograph *module) : ModuleWidget(module){
    {
        DynamicPanel* panel = new DynamicPanel;
        panel->mode = &module->panelStyle;
        addChild(panel);
        box.size = panel->box.size;
    }
    addChild(Widget::create<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(Widget::create<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(Widget::create<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(Widget::create<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addChild(createDynamicText(Vec(69, 83), 14, &module->panelStyle, &module->clockBPM, nullptr, ACTIVE_HIGH));
    addChild(createDynamicText(Vec(27.1,208.5), 14, &module->panelStyle, &module->mapXText, nullptr, ACTIVE_HIGH));
    addChild(createDynamicText(Vec(27.1,268.5), 14, &module->panelStyle, &module->mapYText, nullptr, ACTIVE_HIGH));
    addChild(createDynamicText(Vec(27.1,329), 14, &module->panelStyle, &module->chaosText, nullptr, ACTIVE_HIGH));

    addParam(ParamWidget::create<Rogan1PSBlue>(Vec(49, 40.15), module, Topograph::TEMPO_PARAM, 0.0, 1.0, 0.406));
    addParam(ParamWidget::create<Rogan1PSWhite>(Vec(49, 166.15), module, Topograph::MAPX_PARAM, 0.0, 1.0, 0.0));
    addParam(ParamWidget::create<Rogan1PSWhite>(Vec(49, 226.15), module, Topograph::MAPY_PARAM, 0.0, 1.0, 0.0));
    addParam(ParamWidget::create<Rogan1PSWhite>(Vec(49, 286.15), module, Topograph::CHAOS_PARAM, 0.0, 1.0, 0.0));
    addParam(ParamWidget::create<Rogan1PSBrightRed>(Vec(121, 40.15), module, Topograph::BD_DENS_PARAM, 0.0, 1.0, 0.5));
    addParam(ParamWidget::create<Rogan1PSOrange>(Vec(157, 103.15), module, Topograph::SN_DENS_PARAM, 0.0, 1.0, 0.5));
    addParam(ParamWidget::create<Rogan1PSYellow>(Vec(193, 166.15), module, Topograph::HH_DENS_PARAM, 0.0, 1.0, 0.5));
    addParam(ParamWidget::create<Rogan1PSWhite>(Vec(193, 40.15), module, Topograph::SWING_PARAM, 0.0, 0.9, 0.0));

    addInput(Port::create<PJ301MPort>(Vec(15.5, 48.5), Port::INPUT, module, Topograph::CLOCK_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(15.5, 111.5), Port::INPUT, module, Topograph::RESET_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(15.5, 174.5), Port::INPUT, module, Topograph::MAPX_CV));
    addInput(Port::create<PJ301MPort>(Vec(15.5, 234.5), Port::INPUT, module, Topograph::MAPY_CV));
    addInput(Port::create<PJ301MPort>(Vec(15.5, 294.5), Port::INPUT, module, Topograph::CHAOS_CV));
    addInput(Port::create<PJ301MPort>(Vec(129.5, 234.5), Port::INPUT, module, Topograph::BD_FILL_CV));
    addInput(Port::create<PJ301MPort>(Vec(165.5, 234.5), Port::INPUT, module, Topograph::SN_FILL_CV));
    addInput(Port::create<PJ301MPort>(Vec(201.5, 234.5), Port::INPUT, module, Topograph::HH_FILL_CV));
    addInput(Port::create<PJ301MPort>(Vec(165.5, 48.5), Port::INPUT, module, Topograph::SWING_CV));
    addInput(Port::create<PJ301MPort>(Vec(73, 111.5), Port::INPUT, module, Topograph::RUN_INPUT));

    addOutput(Port::create<PJ3410Port>(Vec(126.7, 270.736), Port::OUTPUT, module, Topograph::BD_OUTPUT));
    addOutput(Port::create<PJ3410Port>(Vec(162.7, 270.736), Port::OUTPUT, module, Topograph::SN_OUTPUT));
    addOutput(Port::create<PJ3410Port>(Vec(198.7, 270.736), Port::OUTPUT, module, Topograph::HH_OUTPUT));
    addOutput(Port::create<PJ3410Port>(Vec(126.7, 306.736), Port::OUTPUT, module, Topograph::BD_ACC_OUTPUT));
    addOutput(Port::create<PJ3410Port>(Vec(162.7, 306.736), Port::OUTPUT, module, Topograph::SN_ACC_OUTPUT));
    addOutput(Port::create<PJ3410Port>(Vec(198.7, 306.736), Port::OUTPUT, module, Topograph::HH_ACC_OUTPUT));

    addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(138.6, 218), module, Topograph::BD_LIGHT));
    addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(174.6, 218), module, Topograph::SN_LIGHT));
    addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(210.6, 218), module, Topograph::HH_LIGHT));

    addParam(ParamWidget::create<LightLEDButton>(Vec(45, 114.5), module, Topograph::RESET_BUTTON_PARAM, 0.0, 1.0, 0.0));
    addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(49.4, 119), module, Topograph::RESET_LIGHT));
    addParam(ParamWidget::create<LightLEDButton>(Vec(102, 114.5), module, Topograph::RUN_BUTTON_PARAM, 0.0, 1.0, 0.0));
    addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(106.4, 119), module, Topograph::RUNNING_LIGHT));
}

struct TopographPanelStyleItem : MenuItem {
    Topograph* module;
    int panelStyle;
    void onAction(EventAction &e) override {
        module->panelStyle = panelStyle;
    }
    void step() override {
        rightText = (module->panelStyle == panelStyle) ? "✔" : "";
        MenuItem::step();
    }
};

struct TopographSequencerModeItem : MenuItem {
    Topograph* module;
    Topograph::SequencerMode sequencerMode;
    void onAction(EventAction &e) override {
        module->sequencerMode = sequencerMode;
        switch(sequencerMode) {
            case Topograph::HENRI:
                module->grids.setPatternMode(PATTERN_HENRI);
                break;
            case Topograph::OLIVIER:
                module->grids.setPatternMode(PATTERN_OLIVIER);
                break;
            case Topograph::EUCLIDEAN:
                module->grids.setPatternMode(PATTERN_EUCLIDEAN);
                break;
        }
    }
    void step() override {
        rightText = (module->sequencerMode == sequencerMode) ? "✔" : "";
        MenuItem::step();
    }
};

struct TopographTriggerOutputModeItem : MenuItem {
    Topograph* module;
    Topograph::TriggerOutputMode triggerOutputMode;
    void onAction(EventAction &e) override {
        module->triggerOutputMode = triggerOutputMode;
    }
    void step() override {
        rightText = (module->triggerOutputMode == triggerOutputMode) ? "✔" : "";
        MenuItem::step();
    }
};

struct TopographAccOutputModeItem : MenuItem {
    Topograph* module;
    Topograph::AccOutputMode accOutputMode;
    void onAction(EventAction &e) override {
        module->accOutputMode = accOutputMode;
        switch(accOutputMode) {
            case Topograph::INDIVIDUAL_ACCENTS:
                module->grids.setAccentAltMode(false);
                break;
            case Topograph::ACC_CLK_RST:
                module->grids.setAccentAltMode(true);
        }
    }
    void step() override {
        rightText = (module->accOutputMode == accOutputMode) ? "✔" : "";
        MenuItem::step();
    }
};

struct TopographClockResolutionItem : MenuItem {
    Topograph* module;
    Topograph::ExtClockResolution extClockResolution;
    void onAction(EventAction &e) override {
        module->extClockResolution = extClockResolution;
        module->grids.reset();
    }
    void step() override {
        rightText = (module->extClockResolution == extClockResolution) ? "✔" : "";
        MenuItem::step();
    }
};

struct TopographRunModeItem : MenuItem {
    Topograph* module;
    Topograph::RunMode runMode;
    void onAction(EventAction &e) override {
        module->runMode = runMode;
    }
    void step() override {
        rightText = (module->runMode == runMode) ? "✔" : "";
        MenuItem::step();
    }
};

void TopographWidget::appendContextMenu(Menu *menu) {
    Topograph *module = dynamic_cast<Topograph*>(this->module);
    assert(module);

    // Panel style
    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Panel style"));
    menu->addChild(construct<TopographPanelStyleItem>(&MenuItem::text, "Dark", &TopographPanelStyleItem::module,
                                                      module, &TopographPanelStyleItem::panelStyle, 0));
    menu->addChild(construct<TopographPanelStyleItem>(&MenuItem::text, "Light", &TopographPanelStyleItem::module,
                                                      module, &TopographPanelStyleItem::panelStyle, 1));

    // Sequencer Modes
    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Sequencer Mode"));
    menu->addChild(construct<TopographSequencerModeItem>(&MenuItem::text, "Henri", &TopographSequencerModeItem::module,
                                                         module, &TopographSequencerModeItem::sequencerMode, Topograph::HENRI));
    menu->addChild(construct<TopographSequencerModeItem>(&MenuItem::text, "Olivier", &TopographSequencerModeItem::module,
                                                         module, &TopographSequencerModeItem::sequencerMode, Topograph::OLIVIER));
    menu->addChild(construct<TopographSequencerModeItem>(&MenuItem::text, "Euclidean", &TopographSequencerModeItem::module,
                                                         module, &TopographSequencerModeItem::sequencerMode, Topograph::EUCLIDEAN));

    // Trigger Output Modes
    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Trigger Output Mode"));
    menu->addChild(construct<TopographTriggerOutputModeItem>(&MenuItem::text, "1ms Pulse", &TopographTriggerOutputModeItem::module,
                                                             module, &TopographTriggerOutputModeItem::triggerOutputMode, Topograph::PULSE));
    menu->addChild(construct<TopographTriggerOutputModeItem>(&MenuItem::text, "Gate", &TopographTriggerOutputModeItem::module,
                                                             module, &TopographTriggerOutputModeItem::triggerOutputMode, Topograph::GATE));

    // Acc Output Modes
    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Accent Output Mode"));
    menu->addChild(construct<TopographAccOutputModeItem>(&MenuItem::text, "Individual accents", &TopographAccOutputModeItem::module,
                                                         module, &TopographAccOutputModeItem::accOutputMode, Topograph::INDIVIDUAL_ACCENTS));
    menu->addChild(construct<TopographAccOutputModeItem>(&MenuItem::text, "Accent / Clock / Reset", &TopographAccOutputModeItem::module,
                                                         module, &TopographAccOutputModeItem::accOutputMode, Topograph::ACC_CLK_RST));

    // External clock resolution
    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Ext. Clock Resolution"));
    menu->addChild(construct<TopographClockResolutionItem>(&MenuItem::text, "4 PPQN", &TopographClockResolutionItem::module,
                                                           module, &TopographClockResolutionItem::extClockResolution, Topograph::EXTCLOCK_RES_4_PPQN));
    menu->addChild(construct<TopographClockResolutionItem>(&MenuItem::text, "8 PPQN", &TopographClockResolutionItem::module,
                                                           module, &TopographClockResolutionItem::extClockResolution, Topograph::EXTCLOCK_RES_8_PPQN));
    menu->addChild(construct<TopographClockResolutionItem>(&MenuItem::text, "24 PPQN", &TopographClockResolutionItem::module,
                                                           module, &TopographClockResolutionItem::extClockResolution, Topograph::EXTCLOCK_RES_24_PPQN));

    // Acc Output Modes
    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Run Mode"));
    menu->addChild(construct<TopographRunModeItem>(&MenuItem::text, "Toggle", &TopographRunModeItem::module,
                                                   module, &TopographRunModeItem::runMode, Topograph::RunMode::TOGGLE));
    menu->addChild(construct<TopographRunModeItem>(&MenuItem::text, "Momentary", &TopographRunModeItem::module,
                                                   module, &TopographRunModeItem::runMode, Topograph::RunMode::MOMENTARY));
}

Model *modelTopograph = Model::create<Topograph, TopographWidget>("Valley", "Topograph", "Topograph", SEQUENCER_TAG);
