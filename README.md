# ESP32 Servo Test

ESP32 Servo tester

Author: Matt Way 2024 (https://github.com/matt-kiwi)

## Requirements
* VSCode with PlatformIO extension. See the following on how to install VSCode/PlatformIO (https://platformio.org/platformio-ide)
* Suitable USB cable
* Servo motor note: adjust timing for servo motor see datasheet
* ESP32 Dev board (https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)

## To use
Connect to ESP32 WiFi access point, default name "ServoTest".
Captive portal should redirect to 192.168.4.1 ,if not manually enter http://192.168.4.1 into your browser.

**Servo datasheets**<br/>
SG90 1.8KG @ 5 volts [SG90 Servo datasheet PDF](docs/SG90_servo_datasheet.pdf)<br/>
DS5160 65KG @ 7.4 volts [DS5160 Servo datasheet PDF](docs/DS5160_servo_datasheet.pdf)<br/>


## License

**MIT License**
 [MIT](https://choosealicense.com/licenses/mit/)

Copyright (c) 2023 Econode NZ Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
