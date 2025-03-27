"""
This module is all about calling C/C++ compilers!
"""

from dataclasses import dataclass
from enum import Enum
from pathlib import Path
import subprocess


class Optimization(Enum):
    O0 = "O0"
    O1 = "O1"
    O2 = "O2"
    O3 = "O3"


class Std(Enum):
    CXX17 = "c++17"


@dataclass
class ExecutableOutput:
    path: Path
@dataclass
class ObjectOutput:
    pass
Output = ExecutableOutput | ObjectOutput


def object_files() -> list[Path]:
    """
    Get all object files in the current directory.
    """
    return list(Path('.').glob('*.o'))


def compile(
    cpp: tuple[Path, ...],
    output: Output,
    opt: Optimization,
    g: bool = False,
    defines: tuple[str, ...] = (),
    std: Std | None = None,
    fsanitize_address: bool = False,
    fsanitize_undefined: bool = False,
    fno_omit_frame_pointer: bool = False,
    include_objects: bool = False,
) -> bool:
    """
    Compile a list of C++ files into an executable.
    """
    cmdline = [
        'g++',
        f"-{opt.value}",
        "-g" if g else "",
        *map(str, cpp),
        *(f"-D{define}" for define in defines),
        f"-std={std.value}" if std else "",
        "-fsanitize=address" if fsanitize_address else "",
        "-fsanitize=undefined" if fsanitize_undefined else "",
        "-fno-omit-frame-pointer" if fno_omit_frame_pointer else "",
        *(object_files() if include_objects else []),
        "-c" if isinstance(output, ObjectOutput) else "",
        "-o" if isinstance(output, ExecutableOutput) else "",
        output.path if isinstance(output, ExecutableOutput) else "",
    ]
    cmdline = list(filter(None, cmdline)) # Only non-empty values
    ret = subprocess.run(cmdline)
    return ret.returncode == 0


def static_library(out: Path):
    """
    Create a static library from all current object files.
    """
    subprocess.run(['ar', 'r', out, *object_files()], check=True)


def remove_object_files():
    """
    Remove all object files in the current directory.
    """
    for p in object_files():
        p.unlink()
