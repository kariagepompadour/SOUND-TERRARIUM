# SOUND TERRARIUM

### *inspired by retro arcade*

**Sound creates the terrain.\
Time and weather create the sky.\
A tiny runner lives inside it.**

**A tiny living world that simulates the present moment of your chosen city.**

## Try SOUND TERRARIUM

▶ **[TRY THE WEB VERSION](https://kariagepompadour.github.io/SOUND-TERRARIUM-Web/)**

🎬 **[WATCH THE DEMO](https://kariagepompadour.github.io/SOUND-TERRARIUM-Web/SOUND_TERRARIUM_demo.mp4)**

**MAKE YOUR OWN SOUND TERRARIUM.**  
**MODIFY IT. REMIX IT. SHARE IT.**  
If I love your idea, it might even become part of the original SOUND TERRARIUM.

SOUND TERRARIUM is open source under the **MIT License**.

<img width="540" height="304" alt="IMG_6223_1951" src="https://github.com/user-attachments/assets/1133b926-5f54-47cf-82cd-0ba96b2522a5" />

SOUND TERRARIUM is a small audio-reactive world for the **M5Stack
Cardputer ADV**.

Set a city anywhere in the world, and SOUND TERRARIUM brings its
present moment into the tiny screen — local time, current weather,
temperature and humidity, atmospheric pressure, precipitation probability, battery status,
sunrise and sunset, moonrise and moonset, tide information, lunar phase,
a reference step count from the built-in BMI270 IMU, and the changing light
from day through twilight into night.

The built-in microphone listens to the sound around the device and
analyzes it in real time. An 8-band graphic equalizer drives the terrain
generator. The EQ display itself is hidden in the normal terrarium view
and can be revealed with **I (Information)**. Whether the bars are visible
or hidden, the same eight-band analysis continues to generate the terrain.
New ground is born at the right edge, scrolls across the screen, and a tiny
runner lives and runs on the landscape created by sound.

At the same time, the sky is connected to the real world. When Wi-Fi is
available, SOUND TERRARIUM uses current time and Open-Meteo data to
reflect weather, sunrise, sunset, moonrise, moonset, lunar phase, atmospheric pressure, precipitation probability, and tide information.

In short:

> **The sound you hear creates the ground.\
> The real world creates the sky.**

------------------------------------------------------------------------

## What makes it different?

SOUND TERRARIUM is not simply a clock with a music visualizer.

The graphic equalizer is part of the world-generation system. The
heights of its eight visible bands become the road itself, so different
music and sounds continuously produce different landscapes.

The runner is therefore not moving over a prerecorded stage. It lives on
terrain being generated in real time.

The Cardputer ADV's IMU is also part of the world: tilting the device
tilts the displayed terrain and affects the runner's movement.

### Step counter

The Cardputer ADV build also uses the **BMI270 built-in Step Counter** to show
**STEP** as a reference value. The count may be reflected after a short delay,
and accuracy varies with how the Cardputer ADV is carried or moved. It is not
intended as a fitness or medical measurement.

**STEP is part of the I (Information) display** and appears at the bottom-left,
directly above **AP / Wi-Fi status** and **LOCATION**. Hold **C for 3 seconds**
to reset the displayed STEP count to **0**.

------------------------------------------------------------------------

## Main features

### Audio-generated terrain

-   Uses the **Cardputer ADV built-in microphone**
-   Real-time spectral analysis from approximately **80--1800 Hz**
-   Eight-band EQ terrain generator; its bars are shown or hidden with **I**
-   Cool-to-warm retro EQ palette
-   The eight EQ band heights directly generate new terrain even while the bars are hidden
-   Generated terrain scrolls from right to left
-   Audio activity also influences the pace of the world
-   Quiet periods fall back smoothly to an idle running pace

### A tiny runner

The runner reacts to the terrain and has several animation states,
including running, jumping, climbing, falling, and waving.

There are also manual and scheduled events, including a UFO sequence. On the
Cardputer ADV build, if tilt carries the runner completely off-screen, there is
a five-second self-recovery window; if the runner is still missing after that,
a UFO rescue sequence automatically comes to bring it back. If dropping the
runner would immediately cause it to slide off-screen again under the current
tilt physics, the UFO keeps the runner safely aboard. The runner is returned to
the terrain once the current device pose no longer causes that slide.

### A sky connected to the real world

When an Internet connection is available, SOUND TERRARIUM uses
**Open-Meteo** data and network time for:

-   Local date and time
-   Current weather
-   Current temperature and relative humidity
-   Mean sea-level pressure
-   Precipitation probability
-   Cloud cover
-   Sunrise and sunset
-   Moonrise and moonset
-   High tide / low tide times and current tide direction
-   Lunar phase
-   Day / night / dawn / dusk changes
-   Location-aware timezone and local clock
-   Daytime Moon visibility that becomes paler as daylight increases

**T** is reserved for the central clock view: local date, weekday, time and
weather. **TEMP** (temperature in °C) and **HUM** (relative humidity in %)
are part of the **I (Information)** display.

More detailed environmental information is treated as auxiliary data:
**PRES** (mean sea-level pressure), **RAIN** (hourly precipitation probability
provided by Open-Meteo for the configured location), **TEMP/HUM**, daily
**SUN/MOON rise/set schedules**, **HIGH/LOW tide times**, current
**TIDE UP / TIDE DN** direction, **STEP**, the saved **LOCATION**, current
**AP / Wi-Fi status**, battery level (**BAT**), and the visible **8-band EQ**. These are hidden by default to keep the 240 × 135
world unobstructed and can be shown or hidden together with the
**I (Information)** key. **I is independent of T**, so auxiliary information
can remain visible even when the normal date/clock/weather overlay is hidden.

The IMU still controls world tilt and runner movement, but its numeric
left/right and front/back angle diagnostics are no longer drawn on the normal
scene. When auxiliary information is shown, **HIGH** and **LOW** tide times are
stacked beneath the SUN/MOON information with a compact blue three-line wave
symbol. At the bottom-left, **STEP** appears above **AP / Wi-Fi status**, with
**LOCATION** below it. At the lower-right, **BAT** appears below the EQ. The EQ
is visible only while **I** is on, but its audio analysis continues unchanged
while hidden, so the sound-generated terrain never stops responding.

The daily Sun/Moon ephemeris is treated separately from frequently
changing weather. After a successful daily fetch, the rise/set values
are retained locally. After the local date changes, SOUND TERRARIUM
requests the new day's values; if that update cannot be obtained, it
keeps the last good values and retries periodically rather than
replacing them with guessed data.

Tide information is obtained separately from the Open-Meteo Marine API. The
next high and low tide times and the current rising/falling direction are
shown as auxiliary information. Tide data is refreshed periodically and, if a
request fails, SOUND TERRARIUM keeps the last valid values when possible and
retries later.

Weather is represented visually with conditions such as:

-   SUNNY
-   CLOUDY
-   RAIN
-   SNOW
-   THUNDER

The Sun and Moon follow low celestial arcs across the display. At rise time,
each body is already visible at the corresponding edge of the screen and then
travels across the sky toward its set edge. The Moon can also appear during
daytime when its rise/set schedule places it above the horizon, but its contrast
is reduced as modeled daylight becomes stronger.

Sky brightness and twilight are driven by a solar-elevation-style visual model
based on the selected location's actual sunrise and sunset, rather than by a
fixed number of minutes before or after those events.

### Offline operation

Wi-Fi is useful, but it is **not required for the core SOUND TERRARIUM
experience**.

Runtime Wi-Fi reconnection is handled without blocking the visual loop. When no saved access point is available, the clock, terrain, runner, audio-reactive scene and controls continue operating while reconnection attempts happen in the background.

Audio analysis, the hidden-or-visible EQ engine, terrain generation, the runner, and the main
animation continue locally on the Cardputer ADV.

Previously obtained time/weather/solar information is retained for
fallback operation where possible.

------------------------------------------------------------------------

## Controls

  Key     Function
  ------- -----------------------------------------------------
  **J**   Jump
  **W**   Wave
  **U**   UFO event
  **T**   Show / hide date, weekday, clock and weather
  **I**   Show / hide detailed information and EQ (SUN/MOON R/S, tide, TEMP/HUM, PRES/RAIN, STEP, AP/LOCATION, BAT and 8-band EQ)
  **C**   Hold for 3 seconds to reset STEP to 0
  **S**   Open Wi-Fi SETUP

The runner also responds to the **Cardputer ADV's IMU**. Tilting the
device left or right tilts the world and can move the runner.

------------------------------------------------------------------------

## Wi-Fi SETUP

SOUND TERRARIUM has its own browser-based Wi-Fi setup system, so
changing networks does not require editing and reflashing the sketch.

Press **S** on the Cardputer ADV.

The display changes to:

**SOUND TERRARIUM**\
*inspired by retro arcade*\
**Wi-Fi SETUP**

The device creates the setup access point:

`SOUND-TERRARIUM-SETUP`

Connect a phone or computer to that Wi-Fi network, then open:

`http://192.168.4.1`

The browser setup page scans nearby Wi-Fi networks and lets you select a
network and enter its password. It also includes **Location (city)**. Enter a
city such as `New York`, `London`, or `Tokyo`; after Wi-Fi connects, SOUND
TERRARIUM uses the Open-Meteo Geocoding API to resolve that city to latitude
and longitude and stores the selected location locally.

The saved location is then used for local weather, temperature, humidity,
sunrise/sunset, moonrise/moonset, and timezone offset. This makes the device
usable worldwide without editing latitude/longitude in the sketch. The same saved location is also used for tide calculations, pressure and precipitation-probability data. If the
location field is left blank, the previously saved location is retained.

SOUND TERRARIUM can store **up to five Wi-Fi networks** and attempts to
connect to a saved network that is available.

While the setup screen is open, the bottom of the Cardputer display
shows:

`PRESS ANY KEY TO EXIT`

Press any physical key to cancel setup and return to normal operation.

> Saved Wi-Fi passwords are not displayed on the setup page.

------------------------------------------------------------------------

## Browser version

The HTML version mirrors the SOUND TERRARIUM world in a 320 × 240 browser
canvas and uses the same Open-Meteo-based location, weather and daily ephemeris
concepts. It provides a **LOCATION SET** field for city-name lookup and, where
the browser allows it, a **USE CURRENT LOCATION** option. The selected location
is saved in browser local storage and drives the displayed local date/time,
weather, temperature/humidity, pressure, precipitation probability, Sun/Moon schedule, and tide information.

Because the browser version is intended as an easy way to try SOUND TERRARIUM,
its interface and instructions are written in **English** for worldwide use.
Because a browser has neither the Cardputer ADV's BMI270 step counter nor its
Wi-Fi access-point state, the browser scene shows **STEP xxxx**, **AP: xxxxxxxx**, and **BAT xx%** as
layout placeholders rather than inventing values.
**Detailed information and the visible EQ are hidden by default** so they do
not cover the generated terrain. Press **I (Information)** to show or hide
SUN/MOON, tide, TEMP/HUM, PRES/RAIN, STEP/AP/LOCATION, BAT, and the 8-band EQ.
The EQ continues to drive the terrain while its bars are hidden.
When shown, **LOCATION appears at the bottom-left** of the browser scene. As on
the Cardputer build, **T and I are independent**: T controls the normal
date/weekday/time/weather overlay, while I controls SUN/MOON, tide, TEMP/HUM,
PRES/RAIN, STEP/AP/LOCATION, BAT, and the visible EQ. **STEP is Cardputer-ADV-only** because
it uses the device's physical BMI270 IMU; the browser version does not simulate
a step count.
Browser geolocation requires permission and may be unavailable in some local-file
or non-secure contexts; city-name lookup remains available. When **USE CURRENT
LOCATION** is used, the browser provides latitude/longitude and a key-free
**BigDataCloud reverse-geocoding endpoint** is used only to obtain a readable
place name when possible; the coordinates remain the fallback if that lookup
fails.

------------------------------------------------------------------------

## Visual design

The visual language deliberately mixes several memories of older
electronic entertainment:

-   classic graphic equalizers
-   arcade games
-   early computer graphics
-   warm sand-colored daytime terrain and primary-blue nighttime terrain
-   retro night skies with mostly white stars, sparse blue/red/yellow stars, and
    a few independently twinkling points
-   simple pixel-like character animation
-   antique human-faced Sun and Moon imagery

The goal is not to reproduce one specific retro machine or game. It is
to create a tiny world that feels as though it might have existed
somewhere between an old arcade cabinet, an audio component, and a
digital clock.

### Where SOUND TERRARIUM came from

This is not a game.

When I was a schoolboy in Japan, **Space Invaders** arrived and changed
what a video game could feel like. For those of us who encountered that
era firsthand, the UFO crossing the top of the screen was not just
another sprite. It was a special event --- unexpected, exciting, and
impossible to forget.

A few years later, as a teenager, I saw **Choplifter!** and **Lode
Runner** running on the **Apple II** in computer stores in Akihabara.
I was stunned. A personal computer could create a world like this.

But machines such as the Apple II were far too expensive for a child
like me. Even if I had somehow been able to own one, I probably would
not have known how to make full use of it. I could watch. Later, with
machines such as the Famicom, I could play. But creating a world of my
own still belonged to another realm.

More than forty years passed.

Then AI arrived.

For the first time, ideas that had lived only in my head could become
programs I could actually build. With AI beside me, I found myself able
to create the kind of tiny moving world that the boy standing in those
computer stores could only dream about.

That is where **SOUND TERRARIUM** came from.

The little runner, the UFO, the cyan, yellow and magenta, the moving
terrain and the tiny sky are not there to reproduce those old games.
They are fragments of the excitement they left behind.

**Back then, I could only watch and play.  
More than forty years later, with the arrival of AI, I could finally
create.**

SOUND TERRARIUM is a tribute to the boy I was a long time ago, and an
expression of gratitude for the fact that I am still here, able to make
something like this.

So do not dismiss it as merely a clock with weather.

**There are dreams from our boyhood inside this little screen.**

------------------------------------------------------------------------

## Hardware

Current target:

-   **M5Stack Cardputer ADV**
-   Built-in microphone
-   Built-in 240 × 135 display
-   Built-in keyboard
-   Built-in BMI270 IMU
-   Wi-Fi

The current microphone implementation uses the Cardputer ADV audio
hardware directly, including the ES8311 codec and ESP-IDF I2S path.

------------------------------------------------------------------------

## Software / services

The current sketch uses Arduino / ESP32 components including:

-   M5Cardputer
-   M5Unified
-   M5GFX / M5Canvas
-   ArduinoJson
-   WiFi
-   WebServer
-   HTTPClient
-   Preferences
-   ESP-IDF I2S API
-   Open-Meteo

Open-Meteo is used for weather and astronomical schedule data, and the Open-Meteo Marine API is used for tide information.

------------------------------------------------------------------------

## Installation

> **This section will be finalized after the current Cardputer ADV build
> has completed its stability testing.**

The current development build is an Arduino `.ino` sketch targeting
**M5Cardputer**.

Before publishing a release, the exact tested Arduino IDE, M5Stack board
package, and library versions will be documented here so that the
installation procedure reflects the actual release environment rather
than an assumed configuration.

------------------------------------------------------------------------

## Design principle

A central rule of SOUND TERRARIUM is that visual effects should have a
reason to exist.

The EQ is not decoration --- it creates the terrain.

The terrain is not random --- it comes from sound.

The sky is not a looping animation --- it reflects the present moment
of the selected city.

The tilt is not a meter --- it changes the world the runner lives in.

That relationship between **sound, reality, and a tiny living world** is
the core of SOUND TERRARIUM.

### BONSAI Spirit

SOUND TERRARIUM follows a **BONSAI Spirit**: a small device containing a small
world, carefully shaped from the capabilities already inside it. The goal is
not to pile on features, but to let each useful capability become a natural
part of the terrarium.

**Small device. Small world. BONSAI Spirit.**

------------------------------------------------------------------------

## Status

SOUND TERRARIUM is currently in active development and real-device
testing on the M5Stack Cardputer ADV.

The present build includes the audio-generated terrain system, runner
animations, IMU interaction, BMI270 reference step counting, real-world weather,
tide and celestial display, scheduled events, offline fallback, worldwide city-based location selection,
and browser-based multi-network Wi-Fi setup.

------------------------------------------------------------------------

## About the name

**SOUND TERRARIUM** describes the idea of keeping a small,
self-contained world inside the Cardputer --- a world whose landscape is
continuously shaped by sound.

*inspired by retro arcade*

------------------------------------------------------------------------

## License, modifications and sharing

SOUND TERRARIUM is released under the **MIT License**. You are welcome to use it,
modify it, experiment with it, and redistribute your own versions under the
terms of the license.

If you create a modified version, I'd appreciate it if you could clearly say
that it is based on SOUND TERRARIUM, describe what you changed, and share your
ideas with the community. This is a request rather than an additional condition
of the MIT License.

My hope is that improvements and new ideas can be seen by others, adapted, and
developed further, so that SOUND TERRARIUM can continue to evolve through the
creativity of everyone who plays with it.

If you publish a modification or improvement under terms that allow it to be
reused, I may incorporate ideas or code from it into the original SOUND
TERRARIUM. When I do, I will give appropriate credit to the contributor where
applicable.

If you make something interesting, **I'd love to see it!**

See [LICENSE](LICENSE) for the full license text.

------------------------------------------------------------------------

**The noise of the city is our energy.\
Run toward tomorrow. Run! Run! Keep running!**
