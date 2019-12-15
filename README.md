# Power_moon

An Arduino controlled power moon with chiptune music and colourful lights
by mit-mit

# Arduino files:

power_moon.ino: main arduino sketch

Tone1.cpp, Tone1.h: modified implementation of Arduino's standard "tone" function for SAMD, re-written to use multiple simultaneous tones on two pins using timers 4 and 5.

pitches.h: contains frequency data for music notes

fossil_falls.h: music data (pitch and durations) for a two-voice arrangement of "Fossil Falls" from Super Mario Odyssey

# Music Files:

fossil_falls.mscx: Musescore (https://musescore.org/en) uncompressed score file for "Fossil Falls" from Super Mario Odyssey (composed by Naoto Kubo, arranged for two-part square waves by mit-mit)

mscx2data.py: Python script used to convert a two-voice musescore file into music data embedded in a header file that can be imported into arduino
