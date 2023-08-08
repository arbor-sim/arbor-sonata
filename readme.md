# SONATA for Arbor

### Build the example and unit tests

#### Optional: Clone and build the arbor library in the `install-arbor` directory

Have Arbor installed somewhere, e.g. /home/you/.local. Alternatively:

```
$ cd install-arbor
$ ./install-local.sh
```

#### Generate unit test input (hdf5 files)

Make sure you have `h5py` installed. Make sure to delete existing files if regenerating, otherwise you seem to end with corrupted h5 files.
```
$ cd test/unit/inputs
$ python generate/gen_edges.py
$ python generate/gen_nodes.py
$ python generate/gen_spikes.py
```

#### Generate example input (hdf5 files)

Make sure you have `h5py` installed. Make sure to delete existing files if regenerating, otherwise you seem to end with corrupted h5 files.
```
$ cd example/network
$ python generate/gen_edges.py
$ python generate/gen_nodes.py
$ cd ../inputs
$ python generate/gen_spikes.py
```

#### Enter the correct paths

* The paths have to be hard coded (for now)
* The following files need to be modified
  * example/circuit_config.json
  * example/network/edge_types.csv
  * example/network/node_types.csv
  * example/simulation_config.json

#### Build the sonata library, unit tests and example

Make sure you have an hdf5 development package installed, e.g. `libhdf5-dev`.
```
$ mkdir build && cd build
$ cmake .. -Darbor_DIR=/absolute/path/to/install-arbor/install/lib/cmake/arbor -DCMAKE_BUILD_TYPE=release && make -j4
```

#### Run the unit tests
```
$ ./build/bin/unit
```

#### Run the example
```
$ ./build/bin/example example/simulation_config.json
```
##### Expected outcome of running the example:
* **2492** spikes generated
* Output spikes report: `output_spikes.h5`
* Voltage and current probe reports: `voltage_report.h5` and `current_report.h5`

### Code org

- `sonata/`
  - contains the library, also libsonata.
- `test/`
  - Unit tests for libsonata
- `example`
  - contains an example application that uses libsonata, and an example Sonata simulation.
  - The example simulation should be split from the application, which is really a Sonata simulation runner.

### Developer notes

- class `sonata_recipe`: public arb::recipe, in `sonata_recipe.hpp`
  - takes `simulation_config.json` as arg
  - main entry point to setting up a Sonata simulation, see `example.cpp`.

- class `sonata_cell` -> arb::cable_cell, in `sonata_cell.hpp`
  - not meant for direct use.

- `example.cpp`
  - An example Sonata simulation
  - Since it takes `simulation_config.json` as input, it's probably not just an example but the main or even only user of libsonata / class `sonata_recipe`.

- class `model_desc`, in `data_management_lib.hpp`
  - member of `sonata_recipe`
  - place where most parsing takes place.
