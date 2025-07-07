#!/usr/bin/env python3
import subprocess
import sys
import os

BACKEND_PREFIX = "game_"
BACKENDS = ["gl", "vk"]

# detect current build directory
BUILD_DIR = os.getcwd()

CMAKE_COMMAND = "cmake"

def build_target(target):
    print(f"Building target: {target} in {BUILD_DIR}")
    result = subprocess.run([CMAKE_COMMAND, "--build", BUILD_DIR, "--target", target])
    if result.returncode != 0:
        print(f"Build failed for target {target}")
        sys.exit(result.returncode)

def usage():
    print(f"Usage:")
    print(f"  {sys.argv[0]} all               # Build all backends")
    print(f"  {sys.argv[0]} <backend>         # Build one backend (e.g. gl or vk)")
    print(f"  {sys.argv[0]} other <backend>   # Build all backends except <backend>")
    sys.exit(1)

def main():
    if len(sys.argv) < 2:
        usage()

    cmd = sys.argv[1]

    if cmd == "all":
        for b in BACKENDS:
            build_target(f"{BACKEND_PREFIX}{b}")

    elif cmd == "other":
        if len(sys.argv) < 3:
            usage()
        skip = sys.argv[2]
        if skip not in BACKENDS:
            print(f"Unknown backend to skip: {skip}")
            sys.exit(1)
        for b in BACKENDS:
            if b != skip:
                build_target(f"{BACKEND_PREFIX}{b}")

    else:
        backend = cmd
        if backend not in BACKENDS:
            print(f"Unknown backend: {backend}")
            sys.exit(1)
        build_target(f"{BACKEND_PREFIX}{backend}")

if __name__ == "__main__":
    main()
