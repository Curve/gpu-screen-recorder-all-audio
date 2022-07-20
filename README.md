# What is this?

This is a small program that allows [gpu-screen-recorder](https://git.dec05eba.com/gpu-screen-recorder/about/) to record audio from both your microphone and headphones at the same time.


**This project requires pipewire to be your audio-system**

# Usage

Record primary display in replay mode
```
$ gpu-screen-recorder-all-audio /usr/bin/gpu-screen-recorder -w "DVI-D-0" -c mp4 -f "60" -r 90 -o "~/Videos/" -a "All Audio (GPU-Screen-Recorder)"
```

**Warning: DVI-D-0 may differ on your system!**

# Compilation

```
$ git clone https://github.com/Curve/gpu-screen-recorder-all-audio
$ cd gpu-screen-recorder-all-audio
$ mkdir build & cd build
$ cmake .. && cmake --build . --config Release