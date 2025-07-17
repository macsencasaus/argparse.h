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
usage: ./example [OPTION]... id [name] [mode]

Example program demonstrating argparse usage

positional arguments:
  id                    the ID to process
  name                  the name to use
  mode {fast,slow,auto} mode to use

options:
  -h, --help            show this help message and exit
  -v, --verbose         enable verbose output
  -r, --retries N       number of retries
  -o, --output FILE     output file name
```
