# ScreenStreaming for the Wii U

> ## ⚠️ STATUS: DOES NOT WORK (as of July 2026) — archived attempt
>
> **This fork does not produce a usable video stream.** I ported the abandoned
> 2018 plugin to modern Aroma and got most of the pipeline working, but the
> actual screen capture never yields a correct image, so it is not usable for
> streaming your Wii U screen. Sharing the findings so others don't repeat the
> same dead ends.
>
> **What works (all fixed/verified on real hardware, Aroma 2026):**
> - Builds against the current toolchain — devkitPPC `20260225` + WUPS
>   `20260418` (the same tags the official `ftpiiu` plugin uses). The old
>   `wiiuwut/libutils:0.1` dependency was dropped (its 2018 ABI caused a
>   `possibly_broken_reent` flag + boot hang); `CThread`/logging are vendored
>   in `src/`.
> - Loads on Aroma **without** the "outdated" warning, and no longer hangs the
>   console at boot.
> - Runs inside games and **exits cleanly** back to the Wii U Menu (fixed a
>   `HeartBeatServer` `join()` deadlock that froze the menu on exit).
> - The **network transport works**: TCP heartbeat (port 8092) + a rewritten,
>   loss-tolerant chunked UDP protocol (per-frame id + chunk index/count, each
>   datagram ≤ MTU). Frames arrive, pass CRC32, and decode at ~10 fps. The
>   matching client change lives in a patched
>   [StreamingPluginClient](https://github.com/Maschell/StreamingPluginClient)
>   (recv buffer + per-frame chunk reassembly).
>
> **What does NOT work (the blocker):**
> - The **GX2 screen capture produces garbage** — the client shows static/noise,
>   not the game. Because the frames pass CRC, the bytes are transmitted
>   perfectly; the plugin is simply encoding the wrong pixels. Capturing at the
>   source's native resolution (instead of a forced downscale, since
>   `GX2CopySurface` does not scale) did **not** fix it, so the issue is deeper
>   in the capture/de-tiling/pixel-format path (`stream_utils.cpp` `copyBuffer`
>   / `EncodingHelper` `convertToJpeg`). This is where a future attempt should
>   focus.
>
> Tested on a real Wii U (US) running Aroma with a WiFi (not wired) connection.
> If you pick this up and get a real image out of the GX2 capture, please open
> an issue — that's the one missing piece.

## Still an early PROOF OF CONCEPT. DON'T EXPECT MAGIC.

This is just a simple plugin that allows you to stream the content of the Gamepad or TV screen to your Computer. With default settings streams in a resolution of 428x240 with selft adjusting quality and tries to achieve as much fps as possible.  
It's possible to adjust the resolution via the config menu (Press **L, DPAD DOWN and MINUS** on your Wii U Gamepad whenever using the home menu is allowed).

But general notes:
- This is still an early PoC.
- Encoding is done in software, not hardware.
- It probably affects gameplay. Loading times are increased, games could lag. I wouldn't recommend you to play online.
- All settings are hardcoded. In the future it will (hopefully) be possible to choose the screen to stream (TV or DRC), at which resolution and which quality.
- No streaming of the home menu.
- Probably unstable.
- Some games might be too dark, some might be too bright, some doesn't work at all.
- Currently streaming is achieved via a custom Java client. 

## Configuration
While the plugin is running, is possible to configure certain parameters. To open the config menu press **L, DPAD DOWN and MINUS** on your Wii U Gamepad. For more information check the [Wii U Plugin System](https://github.com/Maschell/WiiUPluginSystem). 
Currently the following options are available:
- Change the resolution, possible options: 240p, 360p and 480p
- Choose the screen to stream, possible options: Gamepad, TV.


# Usage
Simply load the plugin with the plugin loader, after that start the [StreamingPluginClient](https://github.com/Maschell/StreamingPluginClient). The StreamingPluginClient requires a computer with Java 8. Double click on the `.jar` and enter the IP address of your Wii U console.

If you don't know the IP of your Wii U, you can start for example [ftpiiu](https://github.com/dimok789/ftpiiu) which shows the IP when running.

## Wii U Plugin System
This is a plugin for the [Wii U Plugin System (WUPS)](https://github.com/Maschell/WiiUPluginSystem/). To be able to use this plugin you have to place the resulting `.mod` file into the following folder:

```
sd:/wiiu/plugins
```

When the file is placed on the SDCard you can load it with [plugin loader](https://github.com/Maschell/WiiUPluginSystem/).

## Building

For building you need: 
- [wups](https://github.com/Maschell/WiiUPluginSystem)
- [wut](https://github.com/decaf-emu/wut)
- [libutilswut](https://github.com/Maschell/libutils/tree/wut) (WUT version) for common functions.

Install them (in this order) according to their READMEs. Don't forget the dependencies of the libs itself.

Other external libraries are already located in the `libs` folder.

- libjpeg
- [libturbojpeg](https://libjpeg-turbo.org/)

### Building using the Dockerfile
It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once
docker build . -t screenstreamer-builder

# make 
docker run -it --rm -v ${PWD}:/project screenstreamer-builder make

# make clean
docker run -it --rm -v ${PWD}:/project screenstreamer-builder make clean
```
