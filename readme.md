# SDR++ (sannysanoff fork), The bloat-free SDR software<br>

Please see [upstream project page](https://github.com/AlexandreRouma/SDRPlusPlus) for the main list of its features.

This fork has several goals:

* to add Hermes Lite 2 support
* to experiment with various NOISE REDUCTION algorithms 
* to maintain compatibility with upstream project (regular merges from upstream)
* to add various features that are not in the upstream and have hard time being merged there due to various reasons.

## Installing the fork

Note: Fork may or may not work alongside the original project at present moment. In any case, try. On Windows, should be simpler to run it alongside the original project.

go to the  [recent builds page](https://github.com/sannysanoff/SDRPlusPlus/actions/workflows/build_all.yml), find topmost green build,
click on it, then go to the bottom of the page and find artifact for your system (Linux, Windows, MacOS). It looks like this:
![Example of artifact](https://i.imgur.com/iq8t0Fa.png)

Please note you must be logged in to GitHub to be able to download the artifacts.

## Features compared to original project:

Major:

* Hermes Lite 2 (hl2) support -- separate from recently emerged upstream (hermes) version. Very basic SSB transmit option added.
* FT8/FT4 decoding (experimental) - from MSHV code, moderately tweaked .
* Noise reduction to benefit SSB/AM - wideband and audio frequency. Wideband is visible on the waterfall. Can turn on both. ***Logmmse*** algorithm is used.
* Performance Optimizations:
  * waterfall drawing is greatly optimized at the OpenGL level, important for 4K monitors, eliminating huge CPU/GPU transfers on each frame.
  * waterfall scaling is SSE-friendly, gained 300% speedup, still single-threaded.

Minor:
 
* Mouse wheel scrolling of sliders
* Unicode support in fonts, filenames and installation path (UTF-8), on Windows, too.
* Saving of zoom parameter between sessions
* SNR meter charted below SNR meter - good for comparing antennas
* noise floor calculation differs from original.
* simultaneous multiple audio devices support
* ability to output sound on left or right channel only for particular audio device.
* Airspy HF+ Discovery - narrowing of baseband to hide attenuated parts.
* small screen option + android landscape, makes screen layout more accessible. Tuning knob is there. 

## Version log (newest last)

2022.02.19 initial release. 

* Audio AGC controllable via slider (note: default/original is aggressive) - later original author reimplemented it in a different way 
* Interface scaling - later original author reimplemented it in a different way
* Batch of things from features list (above)

2022.02.20

* for Airspy HF+ Discovery, added Fill-In option which cuts off attenuated far sides of the spectrum.
* added secondary audio stream, so you can output same radio to multiple audio cards / virtual cables
* added single-channel (left/right) option for output audio

2022.06.05

* merging from upstream. Project is still on hold because war in my village (grid myGrid KO80ce).

2022.09.06

* by this time, merge of upstream has been already completed. The log reducion has been greatly optimized compared to previous version.
* you need to enable plugins to use most of fork functionality: noise_reduction_logmmse and hl2_source
* improved waterfall performance

2023.01.07

* added transmit mode for Hermes Lite 2. Note: using my own driver (hl2_), not the one from upstream.
* added mobile screen mode for android/landscape. With big buttons. Has big ambitions, but in early stage.

2023.02.15

* added experimental FT4/FT8 decoding from MSHV code, tweaked for SDR++. All credit goes to MSHV author and down the line.

2023.03.01

* was playing with slow resizing (zooming) of the waterfall. Found a place to optimize it using SSE vector operations in most deep loop. 
  Gained 300% speedup. Still single-threaded, multithreaded will come later.  

## Feedback

``Have an issue? Works worse than original? File an [issue](https://github.com/sannysanoff/SDRPlusPlus/issues).

## Debugging reminders

* to debug in windows in virtualbox env, download mesa opengl32.dll from https://downloads.fdossena.com/Projects/Mesa3D/Builds/MesaForWindows-x64-20.1.8.7z
* make sure you put rtaudiod.dll in the build folder's root otherwise audio sink will not load.
* use system monitor to debug missing dlls while they fail to load.

Good luck.

## Thanks

Thanks and due respect to original author. 
