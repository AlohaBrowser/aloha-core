# Aloha bromium

## Build

1. Build required package `ccache`. If you disable this option, remove line `cc_wrapper` in [args.gn.templ](aloha/build/args.gn.templ).
2. Generate configurations: `python3 aloha/build/gen_build_cfg.py`.
3. Build: `python3 aloha/build/build_webview.py -a <platforms, e.c. 'amd64', optional parameter>`.
4. Make aar: `python3 aloha/build/make_aar.py`.
5. Publication in maven: `python3 aloha/build/publish_aar.py -v <version chromium>-<version bromium>`.
6. AlohaTest: Update gradle in AndroidStudio.
7. AlohaTest: Build and run app.

## Tests
1. Generate tests: `python3 aloha/build/find_tests.py`. Works only after `gen_build_cfg.py`.
2. Build tests: `python3 aloha/build/build_tests.py`.
3. Run tests: `python3 aloha/build/run_tests.py -e <emulator name in adb>`.

## Debug C++ in AndroidStudio

1. Set "Native only" in menu "Edit configuration -> Debugger -> Debug type".
2. Add symbols in the same window in tab "Symbols Directories" path `<your bromium dir>/out/bromium/build/<platform, e.c. 'arm64-v8a'>/lib.unstripped`.
3. Add command `settings set target.source-map ../../../../ "<your bromium dir>/"` in tab "LLDB Startup Commands".
4. Now you can open the C++ file from `<your bromium dir>` and put breakpoint.

## Code compilation clangd in VS code

1. Disable Intelli Sense: Settings -> Search 'C_Cpp.intelliSenseEngine' -> Select 'Disable'.
2. Install vs-code plugin [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd).
3. Generate project configuration by [gen_build_cfg.py](aloha/build/gen_build_cfg.py).
4. Generate clangd config: `tools/clang/scripts/generate_compdb.py -p out/bromium/build/arm64-v8a > compile_commands.json`.
5. Wait for indexing files by clangd-plugin.

## Docker

1. Build docker: `python3 aloha/build/docker_utils.py build`.
2. Run custom command in docker: `python3 aloha/build/docker_utils.py run <command>`. Usually not used.
3. Any aloha python scripts can run in docker image via run with argument `-d`. Example: `python3 aloha/build/build_webview.py -d`.

## Release build and publication

1. Clean release build in docker: `python3 aloha/build/build_clean_release.py -d`.
