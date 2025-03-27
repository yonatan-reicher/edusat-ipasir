from dataclasses import dataclass
from pathlib import Path
import subprocess


tar_name = 'edusat.tar.gz'


def make_linker_file() -> Path:
    p = Path('LINK')
    p.write_text("g++")
    return p


def make_libraries_file() -> Path:
    p = Path('LIBS')
    p.write_text("-lm -lz -lstdc++ -fsanitize=address -fsanitize=undefined")
    return p


def make_makefile(target_name: str) -> Path:
    # Ipasir requires makefile to be lowercase!
    # Makefile requires tabs, spaces will not work.
    p = Path('makefile')
    p.write_text(f"""
all: {target_name}

{target_name}:
\ttar zxf {tar_name} --overwrite

.PHONY: all
    """)
    return p


def clear_dir(p: Path):
    for f in p.iterdir():
        if f.is_dir():
            clear_dir(f)
        else:
            f.unlink()


def make_tar(target: Path) -> Path:
    l1 = make_linker_file()
    l2 = make_libraries_file()
    try:
        subprocess.run(["tar", "zcf", tar_name, target, l1, l2], check=True)
    finally:
        l1.unlink()
        l2.unlink()
    return Path(tar_name)


@dataclass
class Output:
    tar: Path
    makefile: Path


def make_output(target: Path) -> Output:
    return Output(
        tar=make_tar(target),
        makefile=make_makefile(target.name),
    )


def move_to_dir(file: Path, path: Path):
    file.rename(path / file.name)


def pack(target: Path, to_path: Path):
    output = make_output(target)
    to_path.mkdir(exist_ok=True)
    clear_dir(to_path)
    move_to_dir(output.tar, to_path)
    move_to_dir(output.makefile, to_path)
    
