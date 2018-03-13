#include "Nohmad.hpp"

struct LorenzAttractor {
    float sigma, beta, rho, pitch; // Params
    float x, y, z; // Out

    static constexpr float DEFAULT_SIGNMA_VALUE = 10.0;
    static constexpr float DEFAULT_BETA_VALUE = 8.0 / 3.0;
    static constexpr float DEFAULT_RHO_VALUE = 28.0;
    static constexpr float DEFAULT_PITCH_VALUE = 0.5;

    LorenzAttractor() :
        sigma(DEFAULT_SIGNMA_VALUE), beta(DEFAULT_BETA_VALUE), rho(DEFAULT_RHO_VALUE), pitch(DEFAULT_PITCH_VALUE),
        x(1.0), y(1.0), z(1.0) {}

    void process(float dt) {
        float dx = sigma * (y - x);
        float dy = x * (rho - z) - y;
        float dz = (x * y) - (beta * z);

        x += dx * dt * pitch * 375.0;
        y += dy * dt * pitch * 375.0;
        z += dz * dt * pitch * 375.0;
    }
};

struct RosslerAttractor {
    float a, b, c, pitch; // Params
    float x, y, z; // Out

    static constexpr float DEFAULT_A_VALUE = 0.2;
    static constexpr float DEFAULT_B_VALUE = 0.2;
    static constexpr float DEFAULT_C_VALUE = 5.7;
    static constexpr float DEFAULT_PITCH_VALUE = 0.5;

    RosslerAttractor() :
        a(DEFAULT_A_VALUE), b(DEFAULT_B_VALUE), c(DEFAULT_C_VALUE), pitch(DEFAULT_PITCH_VALUE),
        x(1.0), y(1.0), z(1.0) {}

    void process(float dt) {
        float dx = -y - z;
        float dy = x + (a * y);
        float dz = b + z * (x - c);

        x += dx * dt * pitch * 2910.0;
        y += dy * dt * pitch * 2910.0;
        z += dz * dt * pitch * 2910.0;
    }
};

struct StrangeAttractors : Module {
	enum ParamIds {
        LORENZ_SIGMA_PARAM,
        LORENZ_BETA_PARAM,
        LORENZ_RHO_PARAM,
        LORENZ_PITCH_PARAM,
        ROSSLER_A_PARAM,
        ROSSLER_B_PARAM,
        ROSSLER_C_PARAM,
        ROSSLER_PITCH_PARAM,
		NUM_PARAMS
	};

	enum InputIds {
        LORENZ_SIGMA_INPUT,
        LORENZ_BETA_INPUT,
        LORENZ_RHO_INPUT,
        LORENZ_PITCH_INPUT,
        ROSSLER_A_INPUT,
        ROSSLER_B_INPUT,
        ROSSLER_C_INPUT,
        ROSSLER_PITCH_INPUT,
		NUM_INPUTS
	};

	enum OutputIds {
        LORENZ_X_OUTPUT,
        LORENZ_Y_OUTPUT,
        ROSSLER_X_OUTPUT,
        ROSSLER_Y_OUTPUT,
		NUM_OUTPUTS
	};

    LorenzAttractor lorenz;
    RosslerAttractor rossler;

    static constexpr float LORENZ_SIGMA_PARAM_MIN = 3.0;
    static constexpr float LORENZ_SIGMA_PARAM_MAX = 30.0;
    static constexpr float LORENZ_BETA_PARAM_MIN = 0.5;
    static constexpr float LORENZ_BETA_PARAM_MAX = 3.0;
    static constexpr float LORENZ_RHO_PARAM_MIN = 13.0;
    static constexpr float LORENZ_RHO_PARAM_MAX = 80.0;
    static constexpr float LORENZ_PITCH_PARAM_MIN = 0.001;
    static constexpr float LORENZ_PITCH_PARAM_MAX = 1.0;

    static constexpr float ROSSLER_A_PARAM_MIN = 0.0;
    static constexpr float ROSSLER_A_PARAM_MAX = 0.2;
    static constexpr float ROSSLER_B_PARAM_MIN = 0.1;
    static constexpr float ROSSLER_B_PARAM_MAX = 1.0;
    static constexpr float ROSSLER_C_PARAM_MIN = 3.0;
    static constexpr float ROSSLER_C_PARAM_MAX = 12.0;
    static constexpr float ROSSLER_PITCH_PARAM_MIN = 0.001;
    static constexpr float ROSSLER_PITCH_PARAM_MAX = 1.0;

	StrangeAttractors() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}

	void step() override;
};

void StrangeAttractors::step() {
    if (outputs[LORENZ_X_OUTPUT].active || outputs[LORENZ_Y_OUTPUT].active) {
        lorenz.sigma = clampf(params[LORENZ_SIGMA_PARAM].value + inputs[LORENZ_SIGMA_INPUT].value * 0.1, LORENZ_SIGMA_PARAM_MIN, LORENZ_SIGMA_PARAM_MAX);
        lorenz.beta = clampf(params[LORENZ_BETA_PARAM].value + inputs[LORENZ_BETA_INPUT].value * 0.1, LORENZ_BETA_PARAM_MIN, LORENZ_BETA_PARAM_MAX);
        lorenz.rho = clampf(params[LORENZ_RHO_PARAM].value + inputs[LORENZ_RHO_INPUT].value * 0.1, LORENZ_RHO_PARAM_MIN, LORENZ_RHO_PARAM_MAX);
        lorenz.pitch = clampf(params[LORENZ_PITCH_PARAM].value + inputs[LORENZ_PITCH_INPUT].value * 0.1, LORENZ_PITCH_PARAM_MIN, LORENZ_PITCH_PARAM_MAX);

        lorenz.process(1.0 / engineGetSampleRate());
        outputs[LORENZ_X_OUTPUT].value = 5.0 * 0.044 * lorenz.x;
        outputs[LORENZ_Y_OUTPUT].value = 5.0 * 0.0328 * lorenz.y;
    }

    if (outputs[ROSSLER_X_OUTPUT].active || outputs[ROSSLER_Y_OUTPUT].active) {
        rossler.a = clampf(params[ROSSLER_A_PARAM].value + inputs[ROSSLER_A_INPUT].value * 0.1, ROSSLER_A_PARAM_MIN, ROSSLER_A_PARAM_MAX);
        rossler.b = clampf(params[ROSSLER_B_PARAM].value + inputs[ROSSLER_B_INPUT].value * 0.1, ROSSLER_B_PARAM_MIN, ROSSLER_B_PARAM_MAX);
        rossler.c = clampf(params[ROSSLER_C_PARAM].value + inputs[ROSSLER_C_INPUT].value * 0.1, ROSSLER_C_PARAM_MIN, ROSSLER_C_PARAM_MAX);
        rossler.pitch = clampf(params[ROSSLER_PITCH_PARAM].value + inputs[ROSSLER_PITCH_INPUT].value * 0.1, ROSSLER_PITCH_PARAM_MIN, ROSSLER_PITCH_PARAM_MAX);

        rossler.process(1.0 / engineGetSampleRate());
        outputs[ROSSLER_X_OUTPUT].value = 5.0 * 0.054 * rossler.x;
        outputs[ROSSLER_Y_OUTPUT].value = 5.0 * 0.0569 * rossler.y;
    }
}

StrangeAttractorsWidget::StrangeAttractorsWidget() {
	StrangeAttractors *module = new StrangeAttractors();
	setModule(module);
	box.size = Vec(15 * 12, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/StrangeAttractors.svg")));
		addChild(panel);
	}

    addParam(createParam<Davies1900hBlackKnob>(Vec(8, 45), module, StrangeAttractors::LORENZ_SIGMA_PARAM, StrangeAttractors::LORENZ_SIGMA_PARAM_MIN, StrangeAttractors::LORENZ_SIGMA_PARAM_MAX, LorenzAttractor::DEFAULT_SIGNMA_VALUE));
    addParam(createParam<Davies1900hBlackKnob>(Vec(50, 45), module, StrangeAttractors::LORENZ_BETA_PARAM, StrangeAttractors::LORENZ_BETA_PARAM_MIN, StrangeAttractors::LORENZ_BETA_PARAM_MAX, LorenzAttractor::DEFAULT_BETA_VALUE));
    addParam(createParam<Davies1900hBlackKnob>(Vec(92.5, 45), module, StrangeAttractors::LORENZ_RHO_PARAM, StrangeAttractors::LORENZ_RHO_PARAM_MIN, StrangeAttractors::LORENZ_RHO_PARAM_MAX, LorenzAttractor::DEFAULT_RHO_VALUE));
	addParam(createParam<Davies1900hBlackKnob>(Vec(135, 45), module, StrangeAttractors::LORENZ_PITCH_PARAM, StrangeAttractors::LORENZ_PITCH_PARAM_MIN, StrangeAttractors::LORENZ_PITCH_PARAM_MAX, LorenzAttractor::DEFAULT_PITCH_VALUE));
    addParam(createParam<Davies1900hBlackKnob>(Vec(8, 237), module, StrangeAttractors::ROSSLER_A_PARAM, StrangeAttractors::ROSSLER_A_PARAM_MIN, StrangeAttractors::ROSSLER_A_PARAM_MAX, RosslerAttractor::DEFAULT_A_VALUE));
    addParam(createParam<Davies1900hBlackKnob>(Vec(50, 237), module, StrangeAttractors::ROSSLER_B_PARAM, StrangeAttractors::ROSSLER_B_PARAM_MIN, StrangeAttractors::ROSSLER_B_PARAM_MAX, RosslerAttractor::DEFAULT_B_VALUE));
    addParam(createParam<Davies1900hBlackKnob>(Vec(92.5, 237), module, StrangeAttractors::ROSSLER_C_PARAM, StrangeAttractors::ROSSLER_C_PARAM_MIN, StrangeAttractors::ROSSLER_C_PARAM_MAX, RosslerAttractor::DEFAULT_C_VALUE));
	addParam(createParam<Davies1900hBlackKnob>(Vec(135, 237), module, StrangeAttractors::ROSSLER_PITCH_PARAM, StrangeAttractors::ROSSLER_PITCH_PARAM_MIN, StrangeAttractors::ROSSLER_PITCH_PARAM_MAX, RosslerAttractor::DEFAULT_PITCH_VALUE));

	addInput(createInput<PJ301MPort>(Vec(12.5, 110), module, StrangeAttractors::LORENZ_SIGMA_INPUT));
	addInput(createInput<PJ301MPort>(Vec(55, 110), module, StrangeAttractors::LORENZ_BETA_INPUT));
	addInput(createInput<PJ301MPort>(Vec(97.5, 110), module, StrangeAttractors::LORENZ_RHO_INPUT));
	addInput(createInput<PJ301MPort>(Vec(140, 110), module, StrangeAttractors::LORENZ_PITCH_INPUT));
	addInput(createInput<PJ301MPort>(Vec(12.5, 300), module, StrangeAttractors::ROSSLER_A_INPUT));
	addInput(createInput<PJ301MPort>(Vec(55, 300), module, StrangeAttractors::ROSSLER_B_INPUT));
	addInput(createInput<PJ301MPort>(Vec(97.5, 300), module, StrangeAttractors::ROSSLER_C_INPUT));
	addInput(createInput<PJ301MPort>(Vec(140, 300), module, StrangeAttractors::ROSSLER_PITCH_INPUT));

	addOutput(createOutput<PJ301MPort>(Vec(12.5, 154), module, StrangeAttractors::LORENZ_X_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(140, 154), module, StrangeAttractors::LORENZ_Y_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(12.5, 345), module, StrangeAttractors::ROSSLER_X_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(140, 345), module, StrangeAttractors::ROSSLER_Y_OUTPUT));
}
