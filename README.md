
# 🕹️ MCU
#### Dependencies

To build the MCU executable, the following library files must be present in the `/src/lib` directory:

- `json.hpp`: [nlohmann-json v3.12.0](https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp)

#### Building
Build by running `make mcu` from your terminal. If successful, an `mcu` executable will be generated. 


# 🖥️ Faults Panel
The faults panel project is contained wihtin the `/extra_files/faults-panel` subdirectory.
#### Dependencies
- [npm](https://www.npmjs.com/) (known working on v12.2.1)

#### Building
Build by running `make faults-panel` from your terminal inside the `/extra_files/faults-panel` subdirectory. 

If successful, a `.foxe` extension file will be generated. You can load the extension into Foxglove by importing it from the Extensions page.