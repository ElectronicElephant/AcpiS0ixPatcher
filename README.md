ACPIPatcher: S0ix disabler
=======================================

## Dell's Modern Sleep SUCKS!

This repo does exactly the **opposite** to the original repo. Great thanks to @imbushuo.

This simple UEFI application patches your ACPI table to force disable S0 Low Power State 
(aka. [Connected Standby](https://docs.microsoft.com/en-us/windows-hardware/design/device-experiences/modern-standby)) 
regardless of platform configuration. Currently you have to run
it every time before booting into Windows.

**Please note that Windows do not support seamless transition between ACPI S3 and S0ix. A
fresh installation is required.**

## Prerequisites

* [Visual Studio 2017](https://www.visualstudio.com/vs/community/) or gcc/make
* git

## Sub-Module initialization

For convenience, the project relies on the gnu-efi library, so you need to initialize the git
submodule either through git commandline with:
```
git submodule init
git submodule update
```
Or, if using a UI client (such as TortoiseGit) by selecting _Submodule Update_ in the context menu.

## Compilation and testing

Just build project in Visual Studio.

## Visual Studio 2017 and ARM/ARM64 support

Please be mindful that, to enable ARM or ARM64 compilation support in Visual Studio
2017, you __MUST__ go to the _Individual components_ screen in the setup application
and select the ARM/ARM64 compilers and libraries there, as they do __NOT__ appear in
the default _Workloads_ screen:

![VS2017 Individual Components](http://files.akeo.ie/pics/VS2017_Individual_Components2.png)

You also need to ensure that you have Windows SDK 10.0.14393.0 or later installed,
as this is the minimum version with support for ARM64.
