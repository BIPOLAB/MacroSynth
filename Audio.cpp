#include "Audio.h"
#include "Synth.h"
#include "SoundPreset.h"
I2SClass I2S;
void processAudioBlock() {
  consumePendingSoundPreset(audioSoundPreset);
  for (int i = 0; i < AUDIO_FRAMES; i++) {
    float sample = generateSample();

    sample *= masterVolume;
    sample = clampFloat(sample, -1.0f, 1.0f);

    int16_t output = (int16_t)(sample * 32767.0f);

    audioBuffer[i * 2] = output;
    audioBuffer[i * 2 + 1] = output;
  }

  if (audioReady) {
    I2S.write((uint8_t *)audioBuffer, sizeof(audioBuffer));
  }
}


