# TODO 

+ [X] updated bluepad32 from develop, make it compile again
+ [X] figure out why Switch Pro controller crashes bluepad32!!!
+ [X] figure out why mouse events stop coming after a while...

+ [X] port to pico sdk, removing wifi
+ [X] use multiprocess module for processor communication
+ [X] move output control to core 1 completely
+ [X] add power led pwm and state machine
+ [X] enable bluetooth via btstack
+ [X] change core communications to queue_t
+ [ ] send events for mouse and keyboard to core 1
+ [ ] implement abstraction for io port
+ [ ] implement different state machines for different modes of the port
+ [ ] core 1 and 0 coordinate sync and controller assignment behavior, feedback
+ [ ] core 1 operates controller PIO accordingly
+ [X] HW: build first prototype (v1)
+ [X] HW: ~~rev. B:~~ v3:
    - [X] ~~fix dimensions of level shifter daughter boards~~ (not needed anymore, using proper shifter ICs)
    - [X] move Pi Pico USB port to board edge (duh!?)\


## BONUS

+ [ ] add serial port bluetooth, same functionality as usb serial port
+ [ ] integrate pico-flashloader
+ [ ] implement program update from bluetooth serial port


## for V3:

- [X] use same order of bits as mega drive does: -HRL3210
    + pin-wise, TR and TL are inverted. This is to match UART pins for smdt
     this will be handled transparently, internally
- [X] use contiguous pins for ports, enabling usage of PIO
- [ ] reimplement preipheral emulation with PIO
- [X] PCB: use level shifter ICs
- [ ] update docs (readme, new install guide pic.)