# Foot Controller for THR10 Preset Switching

![Footswitch](https://raw.githubusercontent.com/MathisRosenhauer/THR_Footswitch/master/images/thr_footswitch.jpg)

This Arduino sketch uses a USB Host shield to transfer preset data to
one of the [Yamaha THR](http://www.yamaha.com/thr/) guitar amplifiers.
It works similar to the
[Yamaha THR Editor](https://www.youtube.com/watch?v=avRvgELWrFE)
in that it uses the same file format for presets and the same USB (MIDI)
protocol to send the presets to the THR10. Of course you cannot modify
the presets with the sketch as you can do with the THR Editor.

The benefit is that you can fit the Arduino and everything into a pedal
box and switch between presets without the need for a full-sized computer.

The most interesting part is the send_patch() function. It could be used
on different hardware setups with minor changes.

**Note that this only works with the original THR10, THR10C, THR10X,
THR5 and THR5a models. Later models will not work with this sketch
and probably also not with the presented hardware.**

## Hardware

The sketch was written for the following hardware. There are many
alternatives available which may work with the software as well.

- Arduino Uno
- USB Host Shield for Arduino
- SPI Micro SD Card Adapter (optional)
- TM1637 4-digit display

Software libraries used:

- [USB Host Shield 2.0 Library](https://github.com/felis/USB_Host_Shield_2.0)
- [USBH_MIDI](https://github.com/YuuichiAkagawa/USBH_MIDI)
  Only required for Host Shield 2.0 Library below version 1.3.0.
- [SD Library](https://www.arduino.cc/en/Reference/SD) (optional)
- [Arduino library for TM1637](https://github.com/avishorp/TM1637)
- [BOUNCE 2](https://github.com/thomasfredericks/Bounce2)

The sketch also expects two momentary switches to navigate through the
presets stored on a micro SD card or in PROGMEM. All pin assignments
are indicated at the top of [THR_Footswitch.ino](THR_Footswitch.ino).

![look inside](https://raw.githubusercontent.com/MathisRosenhauer/THR_Footswitch/master/images/footswitch_inside.jpg)

If PROGMEM is used to store the presets, then the Python script patchdump.py
may be used to convert presets from a .YDL file to a PROGMEM variable.

`./patchdump.py THR10C.YDL -n 25 > patches.h`

This would put the first 25 presets from THR10C.YDL into
PROGMEM. `USE_SDCARD` then has to be set to `0` in THR_Footswitch.ino to
use PROGMEM for presets.

## May 2019 update

Quite a few people built this project over the past four years and
that is only counting those who contacted me. I'm happy to say that
most people eventually got the footswitch working but there is one
thing that caused trouble: the SD card reader. There seem to be
many card readers out there who don't play nicely on the SPI bus. The
symptom is that the sketch gets stuck because the THR10 is never
recognized by the USB host. Some people also reported that the SD card
itself caused the problems.

When you assemble the project, you should first build the version
without the card reader (presets in PROGMEM). When this works reliably
you can add the card reader. If it then stops working you should try a
different card reader and/or SD card.

I had good results with a Catalex card reader and a Kingston 8 GB
micro SD card but I also got reports that some revisions of the
Catalex reader didn't work.
