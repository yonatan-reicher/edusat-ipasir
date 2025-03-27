from pathlib import Path


def dirs(p: Path) -> list[Path]:
    """ Get the subdirectories of a folder. """
    assert p.is_dir()
    return [x for x in p.iterdir() if x.is_dir()]


def names(p: list[Path]) -> list[str]:
    """ Get the names of a list of paths. """
    return [x.name for x in p]


BinPath = Path
AppPath = Path
SatPath = Path
InputPath = Path


root = Path(".")

results = root / "ipasir-results"
ipasir = root / "ipasir"
src = root / "src"
test_cpp = root / "test.cpp"
test_out = root / "test.out"
# The file to be packed to ipasir after building.
# Note: The 'edusat' in the string must match the string returned by the ipasir interface implementation.
target = root / "libipasiredusat.a"

def src_cpp(): return list(src.rglob("*.cpp"))

apps = ipasir / "app"
solvers = ipasir / "sat"
binaries = ipasir / "bin"
scripts = ipasir / "scripts"

# Path to the script that builds a binary.
mkone = scripts / "mkone.sh"

def app_paths() -> list[AppPath]: return dirs(apps)
def solver_paths() -> list[SatPath]: return dirs(solvers)
def binary_paths() -> list[BinPath]: return [b for b in binaries.iterdir() if b.is_file() and b.name != ".gitignore"]

def inputs(app: AppPath): 
    p = app / "inputs"
    if p.is_dir(): return p
def input_paths(app: AppPath) -> list[InputPath]:
    p = inputs(app)
    return list(p.iterdir()) if p else []


edusat: SatPath = solvers / 'edusat'


def binary_path(app: AppPath, solver: SatPath) -> BinPath:
    return binaries / f'{app.name}-{solver.name}'
