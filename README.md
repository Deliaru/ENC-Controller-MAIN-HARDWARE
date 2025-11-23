| Supported Targets | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | -------- | -------- | -------- |

# TinyUSB WinUSB Vendor Device Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This example implements a WinUSB Vendor device that is compatible with Ontroller.WinUSB.IO library. It provides game controller functionality with button inputs, lever control, and LED outputs.
Upon connection to USB host (PC), the example application will send button and lever data in the format expected by Ontroller.WinUSB.IO, and can receive LED control commands from the PC.

As a USB stack, a TinyUSB component is used.

## How to use example

### Hardware Required

Any ESP board that have USB-OTG supported.

#### Pin Assignment

_Note:_ In case your board doesn't have micro-USB connector connected to USB-OTG peripheral, you may have to DIY a cable and connect **D+** and **D-** to the pins listed below.

See common pin assignments for USB Device examples from [upper level](../../README.md#common-pin-assignments).

GPIO pins are used for button inputs and LED outputs as defined in GPIO_My.h.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```bash
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

After the flashing you should see the output at idf monitor:

```
I (290) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (310) example: USB initialization
I (310) tusb_desc:
┌─────────────────────────────────┐
│  USB Device Descriptor Summary  │
├───────────────────┬─────────────┤
│bDeviceClass       │ 0           │
├───────────────────┼─────────────┤
│bDeviceSubClass    │ 0           │
├───────────────────┼─────────────┤
│bDeviceProtocol    │ 0           │
├───────────────────┼─────────────┤
│bMaxPacketSize0    │ 64          │
├───────────────────┼─────────────┤
│idVendor           │ 0x0E8F      │
├───────────────────┼─────────────┤
│idProduct          │ 0x1216      │
├───────────────────┼─────────────┤
│bcdDevice          │ 0x100       │
├───────────────────┼─────────────┤
│iManufacturer      │ 0x1         │
├───────────────────┼─────────────┤
│iProduct           │ 0x2         │
├───────────────────┼─────────────┤
│iSerialNumber      │ 0x3         │
├───────────────────┼─────────────┤
│bNumConfigurations │ 0x1         │
└───────────────────┴─────────────┘
I (480) TinyUSB: TinyUSB Driver installed
I (480) example: USB initialization DONE
I (2490) WINUSB: WinUSB device initialized successfully
I (3040) WINUSB: WinUSB data sent successfully
```

## Compatibility

This device is designed to be compatible with the Ontroller.WinUSB.IO library:
- Vendor ID: 0x0E8F
- Product ID: 0x1216
- Data format: 7-byte input, 33-byte output
- Protocol: WinUSB Vendor class
