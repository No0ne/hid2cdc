# hid2cdc
USB keyboard to USB CDC converter using a Raspberry Pi Pico

This is currently used to support USB keyboards without PS/2 compatibility on the PicoMiteVGA: https://geoffg.net/picomitevga.html

# Usage
see https://www.thebackshed.com/forum/ViewTopic.php?TID=16545&P=1

| Pin | Function |
|-----|----------|
| GPIO0 | serial debug output at **115200n8** |
| GPIO3 | ctrl-alt-del reset output, optionally connect to **RUN** on the picomite |
| GPIO2 | initial numlock state, jumper to **GND** for numlock **ON** |
| GPIO6 | jumper to **GND** for **QWERTZ keyboard** support |

# Resources
* https://github.com/No0ne/ps2pico
* https://vt100.net/docs/vt100-ug/chapter3.html
* https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/translate.pdf
* https://kbdlayout.info/kbdgr
