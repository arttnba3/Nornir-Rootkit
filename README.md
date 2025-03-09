# Nornir-RootKit

![Illustration by Sumi Kiriko](https://s2.loli.net/2024/12/20/7xqrIvERNb9Tstn.png)

> Illustration by [@SumiKiriko](https://github.com/SumiKiriko), still working in progress...

Yet another modern Linux kernel rootkit for educational purposes.

## Getting started

### O. Verified Installable Platforms

We have tested compiling the `Nornir-Rootkit` on following distros (newest available version). If your computer do not use anyone of them, it's possible that the project could not run successfully, or unexpected problems may occur too.

![](https://img.shields.io/badge/Debian_Bookworm-CE0056?style=for-the-badge&logo=debian&logoColor=white)

![](https://img.shields.io/badge/Gentoo_Linux-54487A?style=for-the-badge&logo=Gentoo&logoColor=white)

![](https://img.shields.io/badge/openSUSE_Tumbleweed-%2364B345?style=for-the-badge&logo=openSUSE&logoColor=white) ![](https://img.shields.io/badge/openSUSE_Slowroll-%2364B345?style=for-the-badge&logo=openSUSE&logoColor=white)

![](https://img.shields.io/badge/openSUSE_Leap_15.5-%2364B345?style=for-the-badge&logo=openSUSE&logoColor=white)

![](https://img.shields.io/badge/Ubuntu_24.04-DD4814?style=for-the-badge&logo=ubuntu&logoColor=white) ![](https://img.shields.io/badge/Ubuntu_22.04-DD4814?style=for-the-badge&logo=ubuntu&logoColor=white) ![](https://img.shields.io/badge/Ubuntu_20.04-DD4814?style=for-the-badge&logo=ubuntu&logoColor=white)

> Note that we hadn't test avaliabilities of all functions on these platforms. Waiting for the future work...

### I. Configure the project

Firstly we need to configure features we'd like to enable in the `Nornir-RootKit`, a graphic configuration menu can be run simply by:

```shell
make menuconfig
```

You can also use the step-by-step guidance, just do the following:

```shell
make config
```

Also, we've also provide a default configuration set. You can use it directly by:

```shell
make defconfig
```

### II. Compile the source code

After the configuration, all you only need to do is:

```shell
make all
```

A loadable kernel module executable binary `nornir.ko` will appear under `src/` .

### III. Installation

You can use following command to load the kernel module into the kernel directly:

```shell
make install
```

Note that currently we do not provide techniques for long-term living yet (bcuz I'm lazy).

## Supported functions

Currently the following functions are supported in `Nornir-RootKit`:

- Hide the module itself
- Grant root privilege if needed
- Hide specific process
- Hide files with specific name
- Hide network connections

Note that we have provided many different techniques in this project to implement some specific functions. For example, the following strategies are available for hidding files:

- Hook the `getdents()` system call directly
- Hook `filldir()`, `filldir64()`, and `compat_filldir()` function
- Hook VFS structures like specific `dir_operations`
- ......

You can refer to the configuration menu (e.g., `menuconfig`) to see all strategies we support.

## Usage

> TODO

## Author

- Code: arttnba3 <arttnba@gmail.com>
- Illustration: 墨 桐子 <kirikosumi@gmail.com>

## License

This project is licensed under the GPL v2 License.

You may obtain a copy of the License at [https://opensource.org/license/gpl-2-0](https://opensource.org/license/gpl-2-0).
