# CERTIFY Low-End RISC-V Node Simulator

This repository provides a simulator for running RV32 applications on the CERTIFY low-end RISC-V node.
The platform is based on a SystemVerilog design of the SoC, processed through Verilator to generate a C++ model. A small library is used to manage the simulation and to interact with the SoC through a basic API.

## Overview

The simulator offers a cycle-accurate model of the node and supports the execution of software developed for the CERTIFY low-end platform. It also allows basic interaction with the SoC to support testing and integration work.

If you want to use this component, or need the full version of the platform, please contact the Trust Up team (info [at] trustup.it).

### Requirements
To use the simulator, the following tools are needed:

- Verilator: Available at: https://github.com/verilator/verilator
- RISC-V Toolchain: Available at: https://github.com/riscv-collab/riscv-gnu-toolchain
- Configure for RV32 as required by your application.
- SRecord (for vmem generation)
- GTKWave (for waveform inspection, if needed)

### Contact
This component is released for free use.
Access to extended material or support is given on request.

To express interest or ask for further information, please contact the CERTIFY team (info [at] trustup.it).

### Copyright
The content of this repository is released for open use under the conditions provided with the project.
Some parts of the full platform remain proprietary. Access to those parts requires explicit permission from TrustUp SRL.
