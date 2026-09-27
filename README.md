# C Virtual Synthesizer 🎹
Small project for a software virtual analog synthesizer written in C for Linux and Windows as a standalone executable and a CLever Audio Plugin (CLAP) usable in REAPER.  
Tested on a Linux Mint Debian Edition machine, a Windows 11 machine and through WSL on Debian.

# Features 🎵
- 3 oscillator synthesizer
- Up to 6 note polyphony (arbitrary, just change the constant in the defs.h header )
- ADSR envelope
- Low pass filter with ADSR envelope
- Detune
- Distortion
- LFO that can modulate either filter cutoff, amplification or detune
- Keyboard input
- MIDI input
- Raygui graphical user interface for standalone mode
- Saving and loading presets configurations XML files
- Recording the synth output into a WAV file
- CLAP virtual instrument plugin usable in REAPER (not tested in other DAWs or plugin hosts, but if they offcially support CLAP plugins it should work, let me know if it doesn't)
- It is possible to bundle the CLAP plugin into a VST3 using [clap-wrapper ](https://github.com/free-audio/clap-wrapper) to use it in DAW that does not support CLAP like Ableton Live, it worked for me using and older version of the [VST3 SDK](https://github.com/steinbergmedia/vst3sdk) (3.8.0)

# GUI 🖼️
## Standalone mode
The GUI shows all of the informations about the synth and let the user configure its parameters graphically :
- Waveform of the sound output
- ADSR envelope parameters level
- Filter ADSR envelope parameters level
- Filter cutoff level
- Amplification level
- Detune level
- Oscillators waveforms
- LFO waveform, modulated parameter and frequency
- Distortion level and overdrive
- Buttons for loading and saving presets into the preset folder
- Button for recording and stop recording into a WAV file
- Piano keyboard showing which keys are being pressed
## Plugin mode
- The GUI of the CLAP plugin is currently unfinished, but you can use the parameter view in REAPER to access every parameter graphically, which may be better than anything I could come up with.

\
![alt text](https://github.com/gpasques-gh/ALSA_raygui_Synthesizer/blob/main/synth.png "GUI screenshot")

# MIDI Input 🎹
The MIDI input should work with all USB MIDI keyboards, to use the synth with your keyboard just follow these two steps :
## Windows
- I did not find a command as simple as `amidi -l` on Linux to list Windows MIDI hardware IDs, so if you want to use MIDI on Windows, just run the synth this way `.\bin\synth -midi 0`. The program should print any available MIDI device and their IDs, then close the program and run it again with the correct ID `.\bin\synth -midi <ID>`.
## Linux
- Using the `amidi -l` command, get your MIDI device hardware id (should look something similar to `hw:1,0,0`)
- Then run the synth : `./bin/synth -midi <hardware id>`

# Keyboard input ⌨️
- The synthesizer can always be played with a computer keyboard, even when using MIDI mode.
- The keyboard simulates a piano keyboard, starting from `a` (`q` in AZERTY) to `j`.  
- The up arrow key move the keys up an octave and the down arrow key move the keys down an octave.
- This current implementation is more useful for testing than it is for playing, I'm planning on adding a second row of keys in the future.

# Dependencies 💻

## Cross-platform
All of these cross-platform dependencies are imported in the project as git submodules, just run `make` for the first time to enable and clone them and don't delete the `external/.submodules-initialized` file to not rebuild them at each compilation.
- raylib
- raygui
- CLAP
- stb_truetype

## Linux
All of these dependencies should be found in any Linux package manager, but they were only tested on APT :
- pulseaudio
- libasound-dev
- zenity
- libxml2

## Windows
The only difference for Windows is that libxml2 is included in the `external` directory, no need for installing or compiling the library.
This project use the GCC compiler so you would need to install it, I use the MinGW-64 one from scoop.
  
# Compilation 🛠️
To compile the projet as a standalone executable : `make` or `make COMPILE_MODE=STANDALONE`.
To compile the project as a CLAP plugin : `make COMPILE_MODE=CLAP`.
Don't forget to add the project directory to the CLAP plugin search path of your DAW.
Create the `presets/` and `audio/` directories in the base project folder in order to use the presets saving and audio recording functionnalities.
  
# Contribute & feedback
Don't hesitate to give feedback and contribute to the project!  
If you encounter any bug (and you most probably will), please report them to me if you have the time!
