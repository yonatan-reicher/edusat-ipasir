"""
This module defines the `cli.start` function, which starts a CLI interface
based on some given configuration.
"""

from dataclasses import dataclass
from functools import cache
from typing import Callable
import os
import sys


# Ahead are a bunch of definitions to define a CLI interface.


@dataclass
class Arg:
    name: str


@dataclass
class Command:
    name: str
    description: str
    function: Callable[..., None]
    args: tuple[Arg, ...] = ()
    variadic_argument: Arg | None = None


@cache
def script_name():
    return os.path.basename(sys.argv[0])


@cache
def args():
    return sys.argv[1:]


@dataclass
class Cli:
    commands: tuple[Command, ...]


def help(cli: Cli):
    table = [
        (
            f"{c.name} {' '.join('<' + a.name + '>' for a in c.args)} {'<' + c.variadic_argument.name + '>...' if c.variadic_argument else ''}",
            f"{c.description}",
        )
        for c in cli.commands
    ]
    left_column_max_width = max(len(left_column) for left_column, _ in table)
    print('Commands:')
    for left, right in table:
        print(f"\t{left:<{left_column_max_width}}\t{right}")


def parse_command(cli: Cli, commandline: str) -> str | Callable[[], None]:
    match commandline.split():
        # Empty command
        case []: return lambda: None
        case [command, *args]:
            for c in cli.commands:
                if command == c.name:
                    if c.variadic_argument:
                        if len(args) < len(c.args):
                            return f"Invalid number of arguments for {command}. Expected at least {len(c.args)}, got {len(args)}."
                    elif len(args) != len(c.args):
                        return f"Invalid number of arguments for {command}. Expected {len(c.args)}, got {len(args)}."
                    return lambda: c.function(*args)
            return f"Unknown command: {command}"
        case _: raise ValueError(f"Invalid argument {commandline=}.")


def handle_command(cli: Cli, commandline: str):
    cmd = parse_command(cli, commandline)
    if callable(cmd):
        cmd()
    else:
        print(cmd)


def start(cli: Cli, hello: str):
    if args() == []:
        print(hello)
        while True:
            try:
                inp = input(">>> ")
                handle_command(cli, inp)
            except KeyboardInterrupt:
                print()
                print('Interrupted.')
                break
            except Exception as e:
                print(f"Error: {e}")

    else:
        for arg in args():
            handle_command(cli, arg)

