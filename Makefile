# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2024 arttnba3 <arttnba@gmail.com>

NORNIR_ROOT_DIR=$(shell pwd)
NORNIR_SRC_DIR=$(NORNIR_ROOT_DIR)/src
LINUX_KERNEL_SRC=/lib/modules/$(shell uname -r)/build

all: .config
	@$(MAKE) -C $(LINUX_KERNEL_SRC) M=$(NORNIR_SRC_DIR) modules

config:
	@$(NORNIR_ROOT_DIR)/scripts/kconf.sh config

defconfig:
	@cp $(NORNIR_SRC_DIR)/configs/x86_64_defconfig $(NORNIR_ROOT_DIR)/.config

menuconfig:
	@$(NORNIR_ROOT_DIR)/scripts/kconf.sh menuconfig

.config:
	@$(MAKE) config

install: all
	@$(insmod) $(NORNIR_SRC_DIR)/nornir.ko

clean:
	@$(MAKE) -C $(LINUX_KERNEL_SRC) M=$(NORNIR_SRC_DIR) clean

.PHONY: clean
