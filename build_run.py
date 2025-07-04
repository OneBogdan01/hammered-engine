import os
import shutil
import subprocess
import sys

build_dir = "build"
generator = "Ninja"
executable_relative_path = os.path.join("bin", "Debug", "game.exe")  # adjust if needed

def run_cmd(cmd, cwd=None):
    print(f"Running: {' '.join(cmd)} (cwd={cwd})")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        print(f"Command failed: {' '.join(cmd)}")
        sys.exit(result.returncode)

def main():
    clean = "--clean" in sys.argv

    if clean and os.path.exists(build_dir):
        print(f"Cleaning build directory '{build_dir}'")
        shutil.rmtree(build_dir)

    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # Configure
    run_cmd(["cmake", "..", "-G", generator], cwd=build_dir)

    # Build
    run_cmd(["cmake", "--build", ".", "--config", "Debug"], cwd=build_dir)

    # Run executable
    exe_path = os.path.join(build_dir, executable_relative_path)
    if not os.path.isfile(exe_path):
        print(f"Executable not found: {exe_path}")
        sys.exit(1)

    print(f"Running executable: {exe_path}")
    # Run executable with cwd set to build directory or wherever DLLs are located
    exe_path = os.path.join(build_dir, "bin", "Debug", "game.exe")
    exe_dir = os.path.dirname(exe_path) 
    run_cmd([exe_path], cwd=exe_dir)

if __name__ == "__main__":
    main()

