#ifndef _MELODYPLAYER_h
#define _MELODYPLAYER_h

#include <Arduino.h>
#include "pitches.h"

struct Tone
{
	int pitch;
	int duration; // 4 = quarter note, 8 = eighth note, etc.
};

class MelodyPlayer
{
public:
	MelodyPlayer(uint8_t speakerPin);

	template<int N>
	void play(const Tone(&melody)[N])
	{
		for (uint8_t i = 0; i < N; i++)
		{
			const int pitch = melody[i].pitch;
			const int duration = 1000 / melody[i].duration;
			const int pause = duration * 1.3;

			tone(speakerPin, pitch, duration);
			delay(pause);
		}
	}

private:
	uint8_t speakerPin;
};

#endif

