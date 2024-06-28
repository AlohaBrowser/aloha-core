# Developer notes

This document includes general information that can be relevant for developers and doesn't fit in any other section.


## Command-line options to enable/disable ad-blocking and Acceptable Ads

In order to test scenarios where ad-blocking or Acceptable Ads need to be disabled by default, you can add certain options to the default Chromium command line:

* Disable ad-blocking: `--disable-eyeo-filtering` or `--disable-adblock` (legacy switch)
* Disable Acceptable Ads: `--disable-aa`

These options can be used when testing a debug build.

For more information about using these options on Android, please check the section below *Setting command-line options on Android*.


## Logging

Logging in Chromium is generally organized in modules, so you can enable verbose logs for some components and not others. The logging principles aren't described in the Chromium documentation, but a general description of the levels can be found [here](https://chromium.googlesource.com/chromium/src/base/+/refs/heads/main/logging.h).

As in many *nix tools, the higher the verbosity level, the more detailed the logs: usually 0 is quiet, 1 is detailed, 2 is very detailed, and 3+ (if used at all) is very low-level debugging output.

### Examples

To enable very verbose logging for all modules:

```sh
--v=2
```

To enable verbose logging for modules related to subscriptions, ad-blocking and converters:

```sh
--vmodule="*subscription*=1,*adblock*=1,*converter*=1"
```

To enable very verbose Telemetry logs related to user counting:

```sh
--vmodule=*telemetry*=2,*activeping*=2
```

For more information about using these options on Android, please check the section below *Setting command-line options on Android*.

### eyeo Chromium SDK logging policy

For easy filtering, eyeo Chromium SDK uses the `[eyeo]` prefix for every log output, as in:

```
DLOG(INFO) << "[eyeo] Annoying ad filtered successfully";
```

The eyeo Chromium SDK policies when it comes to different logging options (release, debug, verbose) are described in detail below. In general, they are designed considering that SDK users prefer not to see a lot of data in their logs regarding content filtering.

#### Release (LOG)

For notifications that are necessary to understand the basics of what is happening in the application, for instance: startup, subscriptions management and user counting. This can be informational, warnings, errors, etc, and should be relevant to partners and QA engineers.

Example:

```
LOG(INFO) << "[eyeo] I want to help with partner integration testing";
```

#### Release verbose (VLOG)

For additional steps in the process. It is more of a step-by-step description that can help a manual QA follow what is happening or where does the application get stuck. Not so much intended for low-level debugging.

:warning: **Note**: These messages are only visible when specifying one of the command line arguments `--v` (level) or `--vmodule` (specific modules). :warning:

#### Debug (DLOG)

For debugging purposes, for instance when an important method starts executing.

#### Debug verbose (DVLOG)

For extra information during debugging, like low-level tracing. This could be operations that support but don't belong to the main flow of the application, like interactions with the filesystem, or very low-level checks during resource filtering.

Example:

```
DVLOG(3) << "[ABP] It is crucial for me to broadcast to everyone";
```

:warning: **Note**: These messages are only visible when specifying one of the command line arguments `--v` (level) or `--vmodule` (specific modules). :warning:


## Setting command-line options on Android

Given that the command line isn't a common interface to interact with an Android device, `adb` is required to add these options to the Chromium command line.

For instance, to disable ad-blocking:

```sh
adb shell 'echo _ --disable-adblock > /data/local/tmp/chrome-command-line'
```

To verify which command line options are currently set:

```sh
adb shell 'cat /data/local/tmp/chrome-command-line'
```

If the options aren't correctly set after running these commands, follow these additional steps before trying to enable them:

1. Install the APK file
2. Open it and go to `chrome://flags`
3. Enable the flag *enable commandline on non-rooted devices*
4. Relaunch the application
5. Kill the application

In order to disable the custom options, just remove the file:

```sh
adb shell "rm /data/local/tmp/chrome-command-line"
```
