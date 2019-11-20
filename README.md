# Credits

This work was done as the course project for SoC Laboratory at the Institute of Computer Technology (ICT) at TU Wien, Vienna, Austria, by

- Johannes Blatnik, e1325707@student.tuwien.ac.at
- Ioannis Daktylidis, e1128193@student.tuwien.ac.at
- Jonas Ferdigg, e1226597@student.tuwien.ac.at
- Daniel Haslauer, e1026814@student.tuwien.ac.at
- Markus Kessler, e1225380@student.tuwien.ac.at
- Edwin Willegger, e1326324@student.tuwien.ac.at

under supervision of Dr. Nima TaheriNejad. For questions or inquiries please contact one of the students (email addresses above) or nima.taherinejad@tuwien.ac.at.

# First steps

This project implements [Nyuzi](https://github.com/jbush001/NyuziProcessor) on a Xilinx Ultrascale+ ZCU102. Please refer to `doc/Documentation.pdf` for a project documentation.

The following steps are required to to build and use the software/hardware:

1) Clone this repository if not already done
2) Get **Nyuzis Toolchain** by executing the following command in the root directory

        git submodule update --init --recursive

3) Read the build instructions for **Nyuzis Toolchain** and build it
4) Build Nyuzi by executing `make` in the subdirectory `sw/nyuzi_sw/`
5) Get the bare-metal application executed by Nyuzi. You can choose between two files:
    1) Use `sw/nyuzi_sw/software/apps/rotozoom/bin/rotozoom.bin` as a very simple example. Executing this file doesn't require any resource file as everything (program code and resource data) is included within one file. If you start this file, some rotating smilies should appear on you screen and Nyuzi starts to send the current FPS via UART to your host PC.
    2) Use `sw/nyuzi_sw/software/apps/scene_viewer/bin/scenev.bin` if you want to load different objects and rotate them.

6) If you want to use the more functional `scene_viewer` application, you have to copy some scenes to the SD card too. Use the binary files from `../apps/cup`, `../apps/pinocchio`, `../apps/pyramide` and `../apps/luigi_circuit`.

> Xilinx's SD card library doesn't support long filenames. It is recommended to use short names like `pino.bin`, `pyra.bin` and so on.

7) Get the hardware bitstream file
    1) This repo provides an already generated bitstream file ready to use. It is stored under the path `syn/ZCU102_BD/GPU_BD_wrapper.bit`.
    2) You can create a new one by opening the Vivado project `syn/ZCU102_BD/Vivado/Nyuzi_BD.xpr`. It is strongly suggested to use **Vivado 2017.4**!

8) Get the software running on the ARM Cortex processor. You can find it under the path `sw/zcu102/scene_change/src/`. 
It's purpose is to list all files stored on the SD card via UART. Using the board pushbuttons, you can select a program file (point 5) and a resource file (point 6), specify the formats (binary or ASCII) and load it to the correct memory location. Nyuzi is released by pressing the middle push button.

9) Get a [PMOD VGA adapter](https://store.digilentinc.com/pmod-vga-video-graphics-array/) and refer to `syn/ZCU102_BD/pinning/pinning.xdc` for a description how the pins must be connected.

10) Connect UART and JTAG cables, insert the SD card to the board, start `XSDK`, program the PL with the bitstream (point 7) and execute the software for the ARM processor (point 8).

11) If you choose the `scene_viewer` application for Nyuzi, control the rotation with the pushbuttons.

> In addition to rotating the objects, you can move within the scene. This is only useful when loading `luigi_circuit`. To do so, set at least one switch to `on` which changes the behavior of the pushbuttons.



## Additional software needed:
`sudo apt install imagemagick-6.q16` to get the convert program, which is used wen generating the resouce files
