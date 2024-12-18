# Nornir-RootKit

Yet another modern Linux kernel rootkit for educational purposes.

## Getting started

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

- arttnba3 <arttnba@gmail.com>

## License

This project is licensed under the GPL v2 License.

You may obtain a copy of the License at [https://opensource.org/license/gpl-2-0](https://opensource.org/license/gpl-2-0).
