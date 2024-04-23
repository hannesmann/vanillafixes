# VanillaFixes

A client modification for World of Warcraft 1.12.1 to eliminate stutter and animation lag.

**Download and installation instructions here:** [Releases](https://github.com/hannesmann/vanillafixes/releases)

![Comparison](docs/comparison.png)

## Compatibility with other mods

#### Other client mods (nampower, etc)

The VanillaFixes launcher has basic support for loading additional DLLs. Add the file to `dlls.txt` and it will be loaded if it can be found in the game directory or at an absolute path. VanillaFixes will show a [reminder](docs/messagebox-dlls.png) when the list of active DLLs has changed.

#### EXE patches (FoV changers, large address patchers, etc)

The VanillaFixes launcher can run the game from a non-standard executable (such as `WoW_tweaked.exe` or `WoWFoV.exe`). There are two ways to accomplish this:

* Drag and drop the executable over the VanillaFixes launcher.
* Create a shortcut to VanillaFixes and add the name of the executable to the end of the "Target" field in "Properties". [Image](docs/shortcut.png)

#### DLL injectors and launchers

These will only work if they load `VfPatcher.dll` before the game process starts (i.e. if the game launches in a suspended state). Injecting at the login screen won't work.

The included launcher is intended to be easy to use and transparent to the user. It should "just work" for the most common cases. For a configurable alternative that works with VanillaFixes and supports multiple game versions, try [wowreeb](https://github.com/namreeb/wowreeb).

## Will I get banned for using VanillaFixes?

As always, you should use tools like these **at your own risk**, but no evidence has been found that VanillaFixes could conflict with the game's anticheat.

VanillaFixes does not modify the game executable, permanently modify any game code, hook any Windows API functions or keep running in the background during gameplay. As soon as you see the login screen it has already been unloaded.

During startup VanillaFixes will modify timing variables in memory to force the game to use a high precision timer. Warden has a check to verify that time functions in the game are behaving as expected (to detect speedhacks, etc). VanillaFixes does not interfere with this check ([test on VMaNGOS](docs/vmangos-timing-check.png)).

## Compiling

VanillaFixes can be built with MSVC or MinGW. A recent version of the Windows SDK (10.0.22000.0+) and [CMake](https://cmake.org) is required. Ninja (https://ninja-build.org) or Visual Studio can be used to build the project.

To build with Ninja, open the x86 VS Native Tools command prompt and run the following commands in the project directory:

```
git submodule update --init

mkdir build
cd build

cmake -G Ninja ..
ninja
```
