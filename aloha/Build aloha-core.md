You can download a prebuilt AAR from our [releases page](https://github.com/AlohaBrowser/aloha-core/releases/), or, if you want to build it yourself, follow the instructions below.

# Building aloha-core

## System requirements

- A Linux x86-64 machine, at least 8 GB of RAM (16 GB or more is recommended).
- At least 200 GB of free disk space.

## Install depot_tools

First, install `depot_tools`. Clone it somewhere under your home directory:

```bash
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

Then add it to your `PATH` by putting the following line in `~/.bashrc` or `~/.zshrc`, so that the `gclient` command becomes available:

```bash
export PATH="$PATH:/path/to/depot_tools"
```

## Prepare the source tree

In the directory that will hold the sources, create a `.gclient` file with the following contents:

```python
solutions = [
  {
    "name": "src",
    "url": "https://chromium.googlesource.com/chromium/src.git",
    "managed": False,
    "custom_deps": {},
    "custom_vars": {
      "checkout_pgo_profiles": True,
    },
  },
]
target_os = ["android"]
```

Next, fetch the aloha-core sources:

```bash
git clone https://github.com/AlohaBrowser/aloha-core
```

Rename the source directory to `src`. You should end up with the following structure:

```
.gclient
src/
  agents/
  aloha/
  android_webview/
  ..........
```

Enter the `src` directory and run:

```bash
gclient sync --no-history
```

This synchronizes all dependencies, after which you can start the build.

## Configure the build

First, in the `aloha/build` directory, run the script that generates the required GN files.

If you don't have CCACHE installed, run the script with the flag below. If CCACHE is installed, it will be used by default:

```bash
./gen_build_cfg.py -c off
```

This configures `BUILD.gn`. You can then build for the target platform of your choice:

```
"arm64": "arm64-v8a",  "arm": "armeabi-v7a",  "x86": "x86"
```

## Build the WebView

As an example, to build for `arm64`:

```bash
./build_webview.py -a arm64
```

This is a good time to take a walk — the build takes a few hours.

## Package the AAR

Once the build finishes, create the AAR that your Android app will consume. From the same `aloha/build` directory, run:

```bash
./make_aar.py
```

The resulting file `aloha-bromium-localbuild.aar` will appear in `out/bromium/aar`.

## Use it in your project

You can now use the AAR in your Android project. 