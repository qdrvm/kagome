# Git Hooks

This folder _might_ contain some git-hooks to execute various checkup in Kagome.

To activate presented (_and all future_) hooks just execute **once** shell-script [activate_hooks.sh](./activate_hooks.sh) from Kagome project root path. 

## pre-commit

This hook check existing `clang-format` and `cmake-format` and their versions.
If they have recommended version, specific checkup is enabled.

Each changed C++ file (`.hpp` and `.cpp` extensions) processing by `clang-format`.

Each changed Cmake file (`CMakeText.txt` and `.cmake` extension) processing by `cmake-format`, and commit blocked if original and formatted file differ.

Commit will be blocked until original and formatted file is differ.

## etc.

_Other hooks might be provided by maintainers in the future._

