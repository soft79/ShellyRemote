# Remote control for Shelly Devices

## Used hardware
- [M5 ATOM Lite ESP32][url_atom] with WIFI (programmable with Arduino IDE)

- Remote control kit [ARD IR REMOTE][url_remote], the IR-receiver fits directly into the M5 ATOM on pins G + 5V + G25.

  ![ARD IR REMOTE!][img_remote]

- Shelly Device(s) that can be controller by http

## Remote Control

- Short press 1..9 to select the device to control
- Long press 1..9 to select the device and immediately toggle power
- Hold LEFT/RIGHT or DOWN/UP to decrease or increase the brightness of the selected device
- Press OK to toggle power of the selected device
- Press "*", "0" or "#" to set brightness to MIN, MIDDLE, MAX (1, 50, 100%)

## Receiver

- The onboard LED shows the current WIFI status. GREEN means connected, RED means disconnected.
- When a supported IR command is received, the corresponding http command will be sent to the Shelly.
  The `?brightness=xxx` or `?turn=toggle` query parameters are appended to the DeviceUrl to control the Shelly.

## Configuration

- Update config_remote.h to use different IR commands or a different input pin
- Update config_shelly.h to configure the remote controlled Shelly devices
- Create config_wifi.private.h (see config_wifi.h) to configure the WIFI SSID + Password

This project is a work in progress, so things will probably change in the future.


[url_atom]: https://shop.m5stack.com/products/atom-lite-esp32-development-kit
[url_remote]: https://www.reichelt.com/arduino-8211-infrared-remote-kit-ard-ir-remote-p282671.html
[img_remote]: Images/ARD_IR_REMOTE.jpg
