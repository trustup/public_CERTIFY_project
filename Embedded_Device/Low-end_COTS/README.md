# UC3 - Artwork Tracking

## Description

This folder provides the code for the **artwork tracking use case**.
UC3 leverages the **STM B-L462E-CELL1 development platform** for cellular IoT devices to:
- Capture environmental sensor data from artworks.
- Send the data to the **artwork tracking server**.
- Integrate with other CERTIFY components, such as the **SIEM** and the **IDS**.

## Features

- **Environmental Sensor Data Collection**  
  Collects sensor readings (e.g., temperature, humidity, etc.) from the STM board and transmits data over cellular networks.

- **Integration with CERTIFY Ecosystem**  
  Sends data to the artwork tracking server, SIEM, and IDS for monitoring and security analysis.  

## Usage

To deploy and use this component:

- **Prerequisites and Dependencies**  
  - STM B-L462E-CELL1 development board.  
  - A SIM card and cellular connectivity.  
  - Artwork tracking server running and accessible.  
  - CERTIFY SIEM and IDS components.  

- **Installation Steps**  
  - Flash the provided code to the STM board.  
  - Configure network credentials for cellular connectivity.  
  - Connect the board to the server and verify communication.  


## Data

The `data/` directory contains:  
- **ids_training_data_uc3.pcap** → Training data for the Network IDS.  
- **snort_ids_logs.txt** → Example IDS logs generated from artwork tracking use case.  
- **sample_artwork_tracking_logs.txt** → Sample sensor logs sent to the artwork tracking server.  

> Sample data is provided in raw formats (PCAP for network traffic, TXT logs for IDS and artwork tracking system).  

## Package

The `package/` directory includes:  
- STM32 project code

## License

This component is distributed under the terms of the **project license agreement**. 

## Contact

For project inquiries:  
- Project Coordinator: **Antonio Skarmeta** - skarmeta@um.es  
- Technical Coordinator: **Stefano Sebastio** - stefano.sebastio@collins.com  
- Repo Maintainer: **Roberto Nardone** - roberto.nardone@uniparthenope.it

