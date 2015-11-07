# Foot Controller for THR10 Preset Switching

![Footswitch](http://4.bp.blogspot.com/-ORFZnIm6-m8/Vj5bSURNuII/AAAAAAAAB0Y/4Z3q43ectlU/s320/thr_footswitch.jpg)

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

## Requirements

The sketch was written for the following hardware. There are many
alternatives available which may work with the software as well.

- Arduino Uno
- USB Host Shield for Arduino
- SPI Micro SD Card Adapter
- TM1637 4-digit display

Software libraries used:

- [USB Host Shield 2.0 Library](https://github.com/felis/USB_Host_Shield_2.0)
- [USBH_MIDI v0.2.0](https://github.com/YuuichiAkagawa/USBH_MIDI) (use latest version!)
- [SD Library](https://www.arduino.cc/en/Reference/SD)
- [Arduino library for TM1637](https://github.com/avishorp/TM1637)
- [BOUNCE 2](https://github.com/thomasfredericks/Bounce2)

The sketch also expects two momentary switches to navigate through the presets
stored on a micro SD card. All pin assignments are indicated at the top of
[THR_Footswitch.ino](THR_Footswitch.ino).

![look inside](http://2.bp.blogspot.com/-dR8Vk3EvQKA/Vj5bcgcDiNI/AAAAAAAAB0g/EX6x3pPKNsU/s320/footswitch_inside.jpg)
