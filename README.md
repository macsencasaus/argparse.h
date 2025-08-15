# argparse.h

Inspired by Python's argparse module: https://docs.python.org/3/library/argparse.html

and tsoding's flag.h: https://github.com/tsoding/flag.h

## Quick Start
Check [example.c](./example.c)
```bash
make
./example -h
```

The output of `./example -h` is
```
usage: ./example [command] [options] id [name] [mode] [files...]

Example program demonstrating argparse usage

commands:
  connect               connect to something
  build                 build program

positional arguments:
  id                    the ID to process
  name                  the name to use
  mode {fast,slow,auto} mode to use
  files                 Files

options:
  -h, --help            show this help message and exit
  -v, --verbose         enable verbose output
  -r, --retries N       number of retries
  -o, --output FILE     output file name
  -L LIB                Linker argument
```
