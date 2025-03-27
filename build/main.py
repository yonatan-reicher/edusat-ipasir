# Standard library imports
from typing import Callable, Literal
from pathlib import Path
import subprocess

# Local imports
from build.cli import Cli, Command, Arg
from build.compile import Optimization, Std, ExecutableOutput, ObjectOutput
from build.paths import binary_path, AppPath, SatPath, BinPath, InputPath, app_paths, solver_paths, binary_paths
import build.cli as cli
import build.compile as compile
import build.pack as pack
import build.paths as paths


def compile_edusat(
    optimized: bool,
    debug: bool = False,
    verbose: Literal[0, 1, 2] = 0,
) -> bool:
    """
    Compile edusat to object files.
    """
    print(f"Compiling edusat... 🛠️ {'(Unoptimized)' if not optimized else ''}")
    defines = (
        "NDEBUG" if optimized else "",
        f"EDUSAT_VERBOSE={verbose}",
        "EDUSAT_DEBUG" if debug else "",
    )
    defines = tuple(filter(None, defines)) # Remove empty strings
    success = compile.compile(
        output=ObjectOutput(),
        cpp=tuple(paths.src_cpp()),
        opt=Optimization.O3 if optimized else Optimization.O0,
        std=Std.CXX17,
        defines=defines,
        fsanitize_address=not optimized,
        fsanitize_undefined=not optimized,
        fno_omit_frame_pointer=not optimized,
    )
    if not success: print("Failed to compile edusat. 💥")
    return success


def make_binary(app: AppPath, solver: SatPath):
    subprocess.run([paths.mkone.absolute(), app.name, solver.name], cwd=paths.ipasir)


def unit_test():
    compile_edusat(optimized=True)
    print('Compiling unit tests... 🧪')
    success = compile.compile(
        output=ExecutableOutput(paths.test_out),
        cpp=(paths.test_cpp,),
        opt=Optimization.O0,
        g=True,
        std=Std.CXX17,
        fsanitize_address=True,
        fsanitize_undefined=True,
        fno_omit_frame_pointer=True,
        include_objects=True,
    )
    if not success: print("Failed to compile unit tests. 💥")
    print('Running')
    subprocess.run([paths.test_out.absolute()], check=True)


def pack_command(
    optimized: bool = True,
    debug: bool = False,
    verbose: Literal[0, 1, 2] = 0,
):
    success = compile_edusat(optimized=optimized, debug=debug, verbose=verbose)
    if not success: return
    compile.static_library(paths.target)
    compile.remove_object_files()
    print('Packing edusat... 📦')
    pack.pack(paths.target, paths.edusat)
    print('Done!')


def pack_debug_command():
    pack_command(optimized=False, verbose=1, debug=True)


def pack_debug_verbose_command():
    pack_command(optimized=False, verbose=2, debug=True)


def parse_arg(
    options: list[Path],
    index_prefix: str,
    error_message: str,
    arg: str,
) -> Path:
    p = next((p for p in options if p.name == arg), None)
    if p: return p
    if arg.startswith(index_prefix):
        index_str = arg[len(index_prefix):]
        if index_str.isdigit():
            index = int(index_str)
            if 1 <= index <= len(options):
                return options[index - 1]
    raise ValueError(error_message)


def parse_app_arg(s: str) -> AppPath:
    return parse_arg(
        paths.app_paths(),
        "a",
        f"App '{s}' does not exist.",
        s,
    )
def parse_sat_arg(s: str) -> SatPath:
    return parse_arg(
        paths.solver_paths(),
        "s",
        f"Solver '{s}' does not exist.",
        s,
    )
def parse_bin_arg(s: str) -> BinPath:
    return parse_arg(
        paths.binary_paths(),
        "b",
        f"Binary '{s}' does not exist.",
        s,
    )
def parse_input_arg(app: AppPath, s: str) -> InputPath:
    return parse_arg(
        paths.input_paths(app),
        "i",
        f"Input '{s}' does not exist for app '{app.name}'.",
        s,
    )


def bin(app_str: str, solver_str: str):
    if app_str == "*":
        solver = parse_sat_arg(solver_str)
        for app in paths.app_paths(): make_binary(app, solver)
    else:
        app = parse_app_arg(app_str)
        solver = parse_sat_arg(solver_str)
        make_binary(app, solver)


def list_command(f: Callable[[], list[Path]]):
    def inner():
        a = f()
        a_len = len(a)
        if a_len == 0:
            print("No items!")
            return
        max_index_width = len(str(a_len))
        for i, item in enumerate(a):
            print(f"{i+1:>{max_index_width}}  {item.name}")
    return inner


def run(app_str: str, sat_str: str, *args):
    app = parse_app_arg(app_str)
    sat = parse_sat_arg(sat_str)
    bin_path = binary_path(app, sat)
    subprocess.run([bin_path, *args])


def run_file(app_str: str, sat_str: str, inp_str: str):
    app = parse_app_arg(app_str)
    sat = parse_sat_arg(sat_str)
    inp = parse_input_arg(app, inp_str)
    bin_path = binary_path(app, sat)
    subprocess.run([bin_path, inp.absolute()])


def inps(app_str: str):
    app = parse_app_arg(app_str)
    print(f"Inputs for {app.name}:")
    list_command(lambda: paths.input_paths(app))()


def ignore(*_):
    pass


hello = f"""
Welcome to the edusat build system!
This CLI tool helps you run and test edusat's incremental implementation, and
run it against various SAT solvers via IPASIR.

You are in interactive mode.
Run `help` to list commands.
Run `exit` to exit.
Note that you may run a command directly without entering this interactive
prompt by running `python3 {cli.script_name()} "<command1>" "<command2>" ...`.
"""


help_inputs = """
Commands can take arguments. Arguments may be strings, or names of applications,
binaries, solvers, or inputs. Each one of these may be listed by it's respective
command. For example, the `apps` command lists all available applications.
Arguments may be abbreviated, by `a<n>`, `b<n>`, `s<n>` or `i<n>` respectivly.
For example, if `edusat` is the 3rd application listed by the `apps` command,
you may refer to it as `a3`.
"""


c = Cli(
    commands=(
        Command(
            name="help",
            description="Get help on some command.",
            function=lambda: cli.help(c),
        ),
        Command(
            name="help-inputs",
            description="Show help on how to input arguments.",
            function=lambda: print(help_inputs),
        ),
        Command(
            name="unit-test",
            description="Runs edusat's unit tests. (test.cpp)",
            function=unit_test,
        ),
        Command(
            name="pack",
            description="Builds edusat and updates ipasir.",
            function=pack_command,
        ),
        Command(
            name="pack-debug",
            description="Like pack, but builds a debug build of edusat.",
            function=pack_debug_command,
        ),
        Command(
            name="pack-debug-verbose",
            description="A very verbose debug build of edusat.",
            function=pack_debug_verbose_command,
        ),
        Command(
            name="bin",
            description="Builds a binary. May pass '*' to `app` to build all binaries with the given solver",
            args=( Arg(name="app"), Arg(name="solver") ),
            function=bin,
        ),
        Command(
            name="apps",
            description="List all available apps.",
            function=list_command(app_paths),
        ),
        Command(
            name="sats",
            description="List all available solvers.",
            function=list_command(solver_paths),
        ),
        Command(
            name="bins",
            description="List all available binaries.",
            function=list_command(binary_paths),
        ),
        Command(
            name="inps",
            description="List all available inputs for an app (Taken from it's source directory).",
            args=( Arg(name="app"), ),
            function=inps,
        ),
        Command(
            name="run",
            description="Run a binary (Does not rebuild it beforehand). `app` and `solver` are used to pick the binary, `arg` are command line arguments that will be passed to the binary.",
            args=( Arg(name="app"), Arg(name="solver") ),
            variadic_argument=Arg(name="arg"),
            function=run,
        ),
        Command(
            name="run-file",
            description="Like `run`, but takes an input file as argument. The input file is passed as an absolute path to the binary via command line arguments. May use shorthand `i<n>` notation (see `help-inputs`).",
            args=( Arg(name="app"), Arg(name="solver"), Arg(name="inp") ),
            function=run_file,
        ),
        Command(
            name="bin-run-file",
            description="Shorthand for `bin`, `run-file`.",
            args=( Arg(name="app"), Arg(name="solver"), Arg(name="inp") ),
            function=lambda app, solver, inp: ignore(bin(app, solver), run_file(app, solver, inp)),
        ),
        Command(
            name="exit",
            description="",
            function=lambda: ignore(print("Goodbye! 👋"), exit(0)),
        ),
    )
)


def main():
    cli.start(c, hello)


if __name__ == '__main__':
    # This should not be called from here as this file is intended to be ran
    # from the build.py file (at the root).
    # But I am leaving this here as convention.
    main()
