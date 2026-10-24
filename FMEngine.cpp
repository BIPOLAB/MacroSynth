#include "FMEngine.h"
#include <math.h>

namespace {
constexpr float kTwoPi = 6.2831853071795864769f;
constexpr float kMinFrequency = 0.1f;
}

FMEngine::FMEngine()
    : sampleRate_(44100.0f),
      baseFrequency_(440.0f),
      velocity_(1.0f),
      feedback_(0.0f),
      operatorCount_(4),
      algorithm_(0) {
  for (uint8_t i = 0; i < MAX_OPERATORS; ++i) {
    operators_[i] = {1.0f, 0.0f, 5.0f, 180.0f, 0.0f, 120.0f};
    state_[i] = {0.0f, 0.0f, 0.0f, 0.0f, ENV_OFF};
  }
  operators_[0] = {1.0f, 1.0f, 3.0f, 250.0f, 0.7f, 180.0f};
  operators_[1] = {2.0f, 0.8f, 2.0f, 180.0f, 0.0f, 100.0f};
  operators_[2] = {3.0f, 0.0f, 5.0f, 200.0f, 0.0f, 100.0f};
  operators_[3] = {4.0f, 0.0f, 5.0f, 200.0f, 0.0f, 100.0f};
}

void FMEngine::begin(float sampleRate) {
  if (sampleRate > 1000.0f) sampleRate_ = sampleRate;
  reset();
}

void FMEngine::reset() {
  for (uint8_t i = 0; i < MAX_OPERATORS; ++i)
    state_[i] = {0.0f, 0.0f, 0.0f, 0.0f, ENV_OFF};
}

void FMEngine::setOperatorCount(uint8_t count) {
  operatorCount_ = count >= 4 ? 4 : 2;
  if (algorithm_ > (operatorCount_ == 2 ? 1 : 4)) algorithm_ = 0;
}

uint8_t FMEngine::operatorCount() const { return operatorCount_; }

void FMEngine::setAlgorithm(uint8_t algorithm) {
  const uint8_t maximum = operatorCount_ == 2 ? 1 : 4;
  algorithm_ = algorithm > maximum ? 0 : algorithm;
}

uint8_t FMEngine::algorithm() const { return algorithm_; }

void FMEngine::setFeedback(float amount) {
  feedback_ = clamp(amount, 0.0f, 0.95f);
}

void FMEngine::setOperator(uint8_t index, const Operator &parameters) {
  if (index >= MAX_OPERATORS) return;
  operators_[index].ratio = clamp(parameters.ratio, 0.125f, 32.0f);
  operators_[index].level = clamp(parameters.level, 0.0f, 1.0f);
  operators_[index].attackMs = clamp(parameters.attackMs, 0.0f, 10000.0f);
  operators_[index].decayMs = clamp(parameters.decayMs, 0.0f, 20000.0f);
  operators_[index].sustain = clamp(parameters.sustain, 0.0f, 1.0f);
  operators_[index].releaseMs = clamp(parameters.releaseMs, 0.0f, 20000.0f);
}

const FMEngine::Operator &FMEngine::getOperator(uint8_t index) const {
  return operators_[index < MAX_OPERATORS ? index : 0];
}

void FMEngine::noteOn(float frequencyHz, float velocity) {
  baseFrequency_ = clamp(frequencyHz, 1.0f, sampleRate_ * 0.20f);
  velocity_ = clamp(velocity, 0.0f, 1.0f);
  for (uint8_t i = 0; i < operatorCount_; ++i) {
    state_[i].phase = 0.0f;
    state_[i].envelope = 0.0f;
    state_[i].releaseStart = 0.0f;
    state_[i].previousOutput = 0.0f;
    state_[i].stage = ENV_ATTACK;
  }
}

void FMEngine::noteOff() {
  for (uint8_t i = 0; i < operatorCount_; ++i) {
    if (state_[i].stage != ENV_OFF && state_[i].stage != ENV_RELEASE) {
      state_[i].releaseStart = state_[i].envelope;
      state_[i].stage = ENV_RELEASE;
    }
  }
}

float FMEngine::renderEnvelope(uint8_t index) {
  OperatorState &s = state_[index];
  const Operator &p = operators_[index];
  const float attackSamples = fmaxf(1.0f, p.attackMs * sampleRate_ * 0.001f);
  const float decaySamples = fmaxf(1.0f, p.decayMs * sampleRate_ * 0.001f);
  const float releaseSamples = fmaxf(1.0f, p.releaseMs * sampleRate_ * 0.001f);

  switch (s.stage) {
    case ENV_OFF: s.envelope = 0.0f; break;
    case ENV_ATTACK:
      s.envelope += 1.0f / attackSamples;
      if (s.envelope >= 1.0f) { s.envelope = 1.0f; s.stage = ENV_DECAY; }
      break;
    case ENV_DECAY:
      s.envelope += (p.sustain - 1.0f) / decaySamples;
      if (s.envelope <= p.sustain) { s.envelope = p.sustain; s.stage = ENV_SUSTAIN; }
      break;
    case ENV_SUSTAIN: s.envelope = p.sustain; break;
    case ENV_RELEASE:
      s.envelope -= s.releaseStart / releaseSamples;
      if (s.envelope <= 0.0f) { s.envelope = 0.0f; s.stage = ENV_OFF; }
      break;
  }
  return s.envelope;
}

float FMEngine::renderOperator(uint8_t index, float phaseModulation) {
  OperatorState &s = state_[index];
  const Operator &p = operators_[index];
  const float frequency = clamp(baseFrequency_ * p.ratio, kMinFrequency, sampleRate_ * 0.45f);
  s.phase = wrapPhase(s.phase + kTwoPi * frequency / sampleRate_);
  const float envelope = renderEnvelope(index);
  const float output = sinf(s.phase + phaseModulation + s.previousOutput * feedback_) *
                       envelope * p.level;
  s.previousOutput = output;
  return output;
}

float FMEngine::renderSample() {
  float output = 0.0f;
  float a, b, c, d;

  if (operatorCount_ == 2) {
    b = renderOperator(1, 0.0f);
    a = renderOperator(0, algorithm_ == 0 ? b * 8.0f : 0.0f);
    output = algorithm_ == 0 ? a : (a + b) * 0.5f;
    return clamp(output * velocity_, -1.0f, 1.0f);
  }

  // Four-op topologies. Operator 0 is the primary carrier.
  switch (algorithm_) {
    case 0: // 3 -> 2 -> 1 -> 0
      d = renderOperator(3, 0.0f);
      c = renderOperator(2, d * 6.0f);
      b = renderOperator(1, c * 5.0f);
      a = renderOperator(0, b * 5.0f);
      output = a;
      break;
    case 1: // (3 -> 2) + (1 -> 0)
      d = renderOperator(3, 0.0f);
      c = renderOperator(2, d * 5.0f);
      b = renderOperator(1, 0.0f);
      a = renderOperator(0, b * 5.0f);
      output = (a + c) * 0.5f;
      break;
    case 2: // (3 + 2) -> 1 -> 0
      d = renderOperator(3, 0.0f);
      c = renderOperator(2, 0.0f);
      b = renderOperator(1, (d + c) * 4.0f);
      a = renderOperator(0, b * 5.0f);
      output = a;
      break;
    case 3: // (3 -> 2) + 1, both modulate 0
      d = renderOperator(3, 0.0f);
      c = renderOperator(2, d * 5.0f);
      b = renderOperator(1, 0.0f);
      a = renderOperator(0, (b + c) * 4.0f);
      output = (a + b) * 0.5f;
      break;
    default: // (3 -> 2 -> 0) + 1
      d = renderOperator(3, 0.0f);
      c = renderOperator(2, d * 5.0f);
      b = renderOperator(1, 0.0f);
      a = renderOperator(0, c * 5.0f);
      output = (a + b) * 0.5f;
      break;
  }
  return clamp(output * velocity_, -1.0f, 1.0f);
}

bool FMEngine::active() const {
  for (uint8_t i = 0; i < operatorCount_; ++i)
    if (state_[i].stage != ENV_OFF) return true;
  return false;
}

float FMEngine::clamp(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

float FMEngine::wrapPhase(float phase) {
  if (phase >= kTwoPi) phase -= kTwoPi;
  else if (phase < 0.0f) phase += kTwoPi;
  return phase;
}
