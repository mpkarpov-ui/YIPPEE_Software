# YIPPEE

**Your Integrated Presicion Positioning (and) Elevation Engine

### About YIPPEE

YIPPEE is a personal project that I started in early 2025, and was my entry into hardware design, as I was interested in learning more about how my [college rocketry team](https://www.illinoisspacesociety.org/spaceshot) developed the hardware for its flight computers.

After receiving help from our Electronic Hardware lead, I had a schematic and layout I was happy with and produced it in our school's electronic lab. The software for this project was in majority written by me with some key contributions from [Surag Nuthulapaty](https://github.com/SuragNuthulapaty), who is a very smart and cool guy.

### Using YIPPEE

#### Hardware

All hardware schematics are present in `hardware/`. As of August 2025, the RPP diode is no longer being produced or sold, but can just be replaced with a jumper wire.

All components can be sourced commercially through Transfer Multisort Elektronik (TME), DigiKey, Mouser, LCSC, and Amazon.


All hardware schematics are free for use.

#### Software

YIPPEE software is compiled with [Platformio](https://platformio.org/), which offers easy VSCode integration with their extension. The configuration defines 4 targets which are described below:

**hardware_test** – This target suspends all normal functions of the flight computer and pre-empts them by putting in code configured to mass-test the components on the board, this is useful for board assembly and debugging.

**main** - This is the main "flight code" and includes finished drivers, flash memory management system, telemetry code, and a barometer-based flight FSM.

**mem_offload** – This target also pre-empts the main flight code, but instead of testing components, it allows offloading data off the flash memory chip.

**ground** (Feather M0) – Running on a commercial-off-the-shelf radio module (the Feather M0 RFM96), this code can receive YIPPEE radio signals and process them to display live board telemetry on the ground.

To start with your YIPPEE board, connect the USB port on the Teensy 4.0 to any COM port on your computer. Then flash the appropriate target through your Platformio interface of choice.

All YIPPEE software is free to reference.
