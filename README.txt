LoliLoader

Windows loader for local mod workflow.

Files:
- `LOAD_MODS.bat` - run script
- `LoliLoader.cpp` - native loader source
- `build.bat` - build script
- `ResolverAgent.jar` - runtime agent
- `mods/` - source jars

Setup:
1. Build agent in project root: `gradlew build`
2. Copy jar: `copy build\libs\ResolverAgent-1.0.jar loader\ResolverAgent.jar`
3. Put mod jars into `mods\`

Run:
- Start `LOAD_MODS.bat`

Build native binary:
- MSVC: `cl /O2 /EHsc /std:c++17 LoliLoader.cpp /link Shell32.lib Advapi32.lib`
- MinGW-w64: `g++ -O2 -std=c++17 -static LoliLoader.cpp -o LoliLoader.exe -lshell32`
- Or run `build.bat`

Environment variables:
- `LOLILOADER_LAUNCHER` - launcher path
- `LOLILOADER_TARGET` - target mods directory
