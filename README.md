# Project - Ipasir version.

Ipasir is an Incremental SAT Solver interface that is used for the sat solving
competition. This repo contains the interfacing of Ofer's EduSAT solver with
the Ipasir interface.

## Building & Running

Before building, run the following to initialize the submodules:
```bash
git submodule update --init --recursive
```

To simplify working on the project, and to minimize the number of things needed
to download and learn to work on it, a python script is used to build, run,
test, and package the project. The script is called `build.py`. It is built for
python version 3.10 or higher. If you get syntax errors, make sure you are
using python 3.10 or higher.

Note that you must be on either Linux or Wsl.

### Example Usage

Build edusat, build genipaessentials with edusat, and run it on it's first
input example:
```bash
./build.py "pack"
./build.py "bin genipaessentials edusat"
./build.py "run-file genipaessentials edusat i1"
```

Note that this can also be written on a single line:
```bash
./build.py "pack" "bin genipaessentials edusat i1" "run-file genipaessentials edusat i1"
```

Or even shorter:
```bash
./build.py "pack" "bin-run-file a2 s1 i1"
```
