#pragma once

#include <stdint.h>

// Allocation-free FM voice for MacroSynth (2 or 4 operators).
// The class is independent of Arduino, I2S and the display.
class FMEngine {
public:
  static constexpr uint8_t MAX_OPERATORS = 4;

  enum Stage : uint8_t { ENV_OFF, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

  struct Operator {
    float ratio;
    float level;
    float attackMs;
    float decayMs;
    float sustain;
    float releaseMs;
  };

  FMEngine();

  void begin(float sampleRate);
  void reset();
  void setOperatorCount(uint8_t count);
  uint8_t operatorCount() const;
  void setAlgorithm(uint8_t algorithm); // 0..4 for four-op; 0..1 for two-op
  uint8_t algorithm() const;
  void setFeedback(float amount); // 0..0.95
  void setOperator(uint8_t index, const Operator &parameters);
  const Operator &getOperator(uint8_t index) const;

  void noteOn(float frequencyHz, float velocity = 1.0f);
  void noteOff();
  float renderSample();
  bool active() const;

private:
  struct OperatorState {
    float phase;
    float envelope;
    float releaseStart;
    float previousOutput;
    Stage stage;
  };

  float sampleRate_;
  float baseFrequency_;
  float velocity_;
  float feedback_;
  uint8_t operatorCount_;
  uint8_t algorithm_;
  Operator operators_[MAX_OPERATORS];
  OperatorState state_[MAX_OPERATORS];

  float renderEnvelope(uint8_t index);
  float renderOperator(uint8_t index, float phaseModulation);
  static float clamp(float value, float low, float high);
  static float wrapPhase(float phase);
};
