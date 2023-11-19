# megaPALadin

This is a switchless region mod for the SEGA Mega Drive, based on the Raspberry Pi Pico.
It uses [arduino-pico](https://github.com/earlephilhower/arduino-pico) as the runtime environment.

![Picture of the mod after installation](doc/installed.jpg)

For now it is quite simple, allowing to change the region of the console via a reset button bypass.
Hold reset for half a second and release, to change desired region. The power LED will blink once for US, twice for EU, and three times for JP. Then do a quick press of the reset button to apply the setting and reset the console. Easy peasy.

Wireless controller and update functionality is planned for the future, when using a Pico W.
That's mostly why it was installed atop the RF shielding, with velcro strips :)

Since this mod maintains the original power LED and communicates the region setting via quick blinking, there's no need to change anything about the external appearance of the console. It should look exactly the same after its done.

An installation diagram is provided [here](doc/install%20plan.png). It is for VA 6.5, check out other switchless mod install guides for other revisions/models.

For more info on this mod, and the thought process, check out the [blog post](https://camargo.eng.br/blog/2023/11-18-pal-megadrive.html).