#include "Audio.h"
#include "Synth.h"
#include "SoundPreset.h"
#include <math.h>
I2SClass I2S;

// Smooth, low-cost saturation for layered drum voices. Unlike a hard clamp,
// this avoids abrupt corners when FM body, noise and transient sum together.
static inline float drumSoftLimit(float x) {
  if (!isfinite(x)) return 0.0f;
  if (x > 3.0f) x = 3.0f;
  else if (x < -3.0f) x = -3.0f;
  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
void processAudioBlock() {
  consumePendingSoundPreset(audioSoundPreset);
  for (int i = 0; i < AUDIO_FRAMES; i++) {
    float sample = generateSample();

    // The BIPO Drum voice applies its per-sound level internally; other legacy
    // engines continue to use the global output level.
    if (currentEngine != ENGINE_KICK) {
      sample *= masterVolume;
      sample = clampFloat(sample, -1.0f, 1.0f);
    } else {
      sample = drumSoftLimit(sample);
    }

    int16_t output = (int16_t)(sample * 32767.0f);

    audioBuffer[i * 2] = output;
    audioBuffer[i * 2 + 1] = output;
  }

  if (audioReady) {
    I2S.write((uint8_t *)audioBuffer, sizeof(audioBuffer));
  }
}


