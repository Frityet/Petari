from pathlib import Path

import pyghidra
from jpype import JLong


ROOT = Path(__file__).resolve().parents[3]
PROJECT_DIR = Path("/tmp/mario-walk-ghidra-elf-7")
FUNCTIONS = (
    0x802DB028,
    0x802DB0C8,
    0x802DB1E0,
)


pyghidra.start(install_dir=Path("/opt/ghidra"))

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor


with pyghidra.open_program(
    ROOT / "build/RMGK02/main.elf",
    project_location=PROJECT_DIR,
    project_name="RMGK02",
    analyze=False,
) as api:
    program = api.currentProgram
    decompiler = DecompInterface()
    decompiler.openProgram(program)
    monitor = ConsoleTaskMonitor()

    functions = program.getFunctionManager()
    output = [f"image: {program.getMinAddress()}..{program.getMaxAddress()}"]
    for address in FUNCTIONS:
        function = functions.getFunctionAt(api.toAddr(JLong(address)))
        if function is None:
            output.append(f"/* missing function at {address:#x} */")
            continue
        result = decompiler.decompileFunction(function, 180, monitor)
        output.append(f"/* {function.getEntryPoint()}: {function.getName()} */")
        output.append(result.getDecompiledFunction().getC())

    print("\n".join(output))
