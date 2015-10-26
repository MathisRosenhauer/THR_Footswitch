/*
 * Arduino program for sending preset data to a Yamaha THR10 guitar
 * amplifier.  Copyright (C) 2015 Mathis Rosenhauer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, If not, see <http://www.gnu.org/licenses/>
*/

#include <Bounce2.h>
#include <SD.h>
#include <Usb.h>
#include <usbh_midi.h>
#include <TM1637Display.h>

// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#endif

const uint8_t button1_pin = 2;
const uint8_t button2_pin = 3;
const uint8_t sdcard_ss_pin = 4;
const uint8_t display_dio_pin = 5;
const uint8_t display_clk_pin = 6;
// usbh_ss_pin 10
// usbh_int_pin 9

USB Usb;
USBH_MIDI Midi(&Usb);
TM1637Display display(display_clk_pin, display_dio_pin);
Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();

void setup()
{
    Serial.begin(115200);

    // Buttons setup
    pinMode(button1_pin,INPUT_PULLUP);
    debouncer1.attach(button1_pin);
    debouncer1.interval(5);
    pinMode(button2_pin,INPUT_PULLUP);
    debouncer2.attach(button2_pin);
    debouncer2.interval(5);
    display.setBrightness(0x0f);

    // SD card setup
    if (!SD.begin(sdcard_ss_pin)) {
        Serial.println("Card failed, or not present");
        return;
    }
    Serial.println("Card initialized.");

    // Workaround for non UHS2.0 Shield
    pinMode(7,OUTPUT);
    digitalWrite(7,HIGH);

    // USBH setup
    if (Usb.Init() == -1) {
        while(1); //halt
    }
    delay(200);
    Serial.println("USB initialized.");
}

void send_patch(uint8_t patch_id)
{
    int i, j;
    uint8_t prefix[] = {
        0xf0, 0x43, 0x7d, 0x00, 0x02, 0x0c, 0x44, 0x54,
        0x41, 0x31, 0x41, 0x6c, 0x6c, 0x50, 0x00, 0x00,
        0x7f, 0x7f
    };
    int cs = 0x371; // prefix checksum
    const int patch_size = 0x100;
    uint8_t line[16];
    uint8_t suffix[2];
    File patchfile;

    patchfile = SD.open("THR10C.YDL");
    if (patchfile) {
        Midi.SendSysEx(prefix, sizeof(prefix));

        patchfile.seek(0xd + (patch_id - 1) * (patch_size + 5));
        for (j = 0; j < patch_size; j += sizeof(line)) {
            for (i = 0; i < sizeof(line); i++) {
                if (j + i == patch_size - 1)
                    line[i] = 0;
                else
                    line[i] = patchfile.read();
                cs += line[i];
            }
            Midi.SendSysEx(line, sizeof(line));
        }
        suffix[0] = (~cs + 1) & 0x7f;
        suffix[1] = 0xf7;
        Midi.SendSysEx(suffix, sizeof(suffix));
        patchfile.close();
    } else {
        Serial.println("File not present.");
    }
}

int read_buttons(void)
{
    debouncer1.update();
    debouncer2.update();
    return (debouncer1.rose() ? 1: 0) + (debouncer2.rose() ? -1: 0);
}

void loop() {
    static uint8_t patch_id = 1;
    static uint16_t pid = 0;
    static uint16_t vid = 0;
    static bool thr_connected = false;
    int button_state;

    Usb.Task();
    if(Usb.getUsbTaskState() == USB_STATE_RUNNING) {
        if(Midi.vid != vid || Midi.pid != pid){
            vid = Midi.vid;
            pid = Midi.pid;
            thr_connected = true;
            send_patch(patch_id);
            display.showNumberDec(patch_id, false);
        }
    }

    button_state = read_buttons();
    if (button_state != 0 && thr_connected) {
        patch_id = constrain(patch_id + button_state, 0, 100);
        send_patch(patch_id);
        display.showNumberDec(patch_id, false);
    }
}
