## Development

This repository contains the Guppy Screen source code and all its external dependencies.

Dependencies:
 - [lvgl](https://github.com/lvgl/lvgl)
   An embedded graphics library
 - [libhv](https://github.com/ithewei/libhv)
   A network library
 - [spdlog](https://github.com/gabime/spdlog)
   A logging library
 - [wpa_supplicant](https://w1.fi/wpa_supplicant/)
   Handles wireless connections

## Toolchains
The Guppy Screen uses features (filesystem) from C++17, so a gcc/g++ version (7.2+) with C++17 support is required.

### Environment Variables
`CROSS_COMPILE` - The prefix to the toolchain architecture, e.g. `mips-linux-gnu-`  
`SIMULATION` - Define it to build with SDL for running on your local machine.  
`ZBOLT` - Define it to use the Z-Bolt icon set. By default the build uses the Material Design Icons.  
`GUPPYSCREEN_VERSION` - Version string displayed in the System Panel in the UI.

### Build Environment

#### Ubuntu and Debian
For Ubuntu/Debian install build essentials and libsdl2-dev packages.

`sudo apt-get install -y build-essential cmake libsdl2-dev`

#### Arch and Derivatives

For Arch and derivatives install 'base-devel' and 'sdl2' packages.

`sudo pacman -S base-devel cmake sdl2`

#### Mipsel Tool chain

To build guppyscreen for Mipsel (Ingenic X2000E) - specific to the K1 SoC, you will need the mips-gcc720 tool chain.

1. Download the toolchain [here](https://github.com/ballaswag/k1-discovery/releases/download/1.0.0/mips-gcc720-glibc229.tar.gz)
2. `tar xf mips-gcc720-glibc229.tar.gz && export PATH=<path-to-mips-toolchain/bin>:$PATH`

### The Code

Clone the guppyscreen repo (and submodules) and apply a couple of patches locally.

1. `git clone --recursive https://github.com/ballaswag/guppyscreen && cd guppyscreen`
2. `(cd lv_drivers/ && git apply ../patches/0001-lv_driver_fb_ioctls.patch)`
3. `(cd spdlog/ && git apply ../patches/0002-spdlog_fmt_initializer_list.patch)`

### Mipsel (Ingenic X2000E) - specific to the K1 SoC
Building for the K1/Max

1. `export CROSS_COMPILE=mips-linux-gnu-`
2. `make clean && make -j$(nproc) build`

After an initial `make build`, you can make changes to src guppy files and then use `make` to compile the files that need compiling.

The executable is ./build/bin/guppyscreen

### x86_64 (Intel/AMD)
Building and running Guppy Screen on your local machine speeds up development. Changes can be tested on the local machine before rebuilding for the other architectures.

1. `unset CROSS_COMPILE`
2. `make clean && make -j$(nproc) build`

After an initial `make build`, you can make changes to src guppy files and then use `make` to compile the files that need compiling.

The executable is ./build/bin/guppyscreen

### Simulation
Guppy Screen default configurations (guppyconfig.json) is configured for the K1/Max. In order to run it remotely as a simulator build, a few thing needs to be setup.
The following attributes need to be configured in `build/bin/guppyconfig.json`

1. `log_path` - Absolute path to `guppyscreen.log`. Directory must exist locally.
2. `thumbnail_path` - Absolute path to a local directory for storing gcode thumbnails.
3. `moonraker_host` - Moonraker IP address
4. `moonraker_port` - Moonraker Port
5. `wpa_supplicant` - Path to the wpa_supplicant socket (usually under /var/run/wpa_supplicant/)

```
{
  "default_printer": "k1",
  "log_path": "<local_path_to_guppyscreen.log>",
  "printers": {
    "k1": {
      "display_sleep_sec": 300,
      "moonraker_api_key": false,
      "moonraker_host": "<remote_ip_to_moonraker>",
      "moonraker_port": <moonraker_port_if_not_7125>
    }
  },
  "thumbnail_path": "<local_path_to_thumbnail_directory_for_storing_gcode_thumbs>",
  "wpa_supplicant": "<path_to_the_wireless_interface_wpa_supplicant_socket-e.g. /var/run/wpa_supplicant/wlo1>"
}

```

Note: Guppy Screen currently requires running as `root` because it directly interacts with wpa_supplicant.

### Virtual Klipper

It is possible to use https://github.com/mainsail-crew/virtual-klipper-printer to start a virtual printer locally
to make local testing and development easier.   You will need to install docker-ce and docker-compose locally.   

### Install Docker and Docker Compose

#### Ubuntu and Debian

You can follow the instructions to get docker and docker-compose setup on Ubuntu:
https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository

1. `sudo apt-get update && sudo apt-get install ca-certificates curl gnupg`
2. `sudo install -m 0755 -d /etc/apt/keyrings`
3. `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg`
4. `sudo chmod a+r /etc/apt/keyrings/docker.gpg`
3. `echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null`
4. `sudo apt-get update`
5. `sudo apt-get install docker-ce docker-compose docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin`

#### For Arch and Derivatives

1. `sudo pacman -S docker docker-compose`
2. `sudo systemctl start docker`

#### Build and Start

1. `git clone https://github.com/mainsail-crew/virtual-klipper-printer.git && cd virtual-klipper-printer`
2. `sudo docker-compose up -d`

You can now configure the guppyconfig.json `moonraker_host` to be `127.0.0.1` and `moonraker_port` to be 7125
