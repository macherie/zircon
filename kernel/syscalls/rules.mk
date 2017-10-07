# Copyright 2016 The Fuchsia Authors
# Copyright (c) 2008-2015 Travis Geiselbrecht
#
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
    kernel/lib/console \
    kernel/lib/crypto \
    kernel/lib/fbl \
    kernel/lib/pci \
    kernel/lib/user_copy \
    kernel/lib/vdso \
    kernel/object \

MODULE_SRCS := \
    $(LOCAL_DIR)/syscalls.cpp \
    $(LOCAL_DIR)/syscalls_channel.cpp \
    $(LOCAL_DIR)/syscalls_ddk.cpp \
    $(LOCAL_DIR)/syscalls_ddk_pci.cpp \
    $(LOCAL_DIR)/syscalls_debug.cpp \
    $(LOCAL_DIR)/syscalls_exceptions.cpp \
    $(LOCAL_DIR)/syscalls_fifo.cpp \
    $(LOCAL_DIR)/syscalls_futex.cpp \
    $(LOCAL_DIR)/syscalls_handle_ops.cpp \
    $(LOCAL_DIR)/syscalls_hypervisor.cpp \
    $(LOCAL_DIR)/syscalls_zircon.cpp \
    $(LOCAL_DIR)/syscalls_object.cpp \
    $(LOCAL_DIR)/syscalls_object_wait.cpp \
    $(LOCAL_DIR)/syscalls_port.cpp \
    $(LOCAL_DIR)/syscalls_resource.cpp \
    $(LOCAL_DIR)/syscalls_socket.cpp \
    $(LOCAL_DIR)/syscalls_system.cpp \
    $(LOCAL_DIR)/bootdata_unittest.cpp \
    $(LOCAL_DIR)/syscalls_task.cpp \
    $(LOCAL_DIR)/syscalls_test.cpp \
    $(LOCAL_DIR)/syscalls_timer.cpp \
    $(LOCAL_DIR)/syscalls_vmar.cpp \
    $(LOCAL_DIR)/syscalls_vmo.cpp \

ifeq ($(ARCH),x86)
MODULE_SRCS += $(LOCAL_DIR)/syscalls_system_x86.cpp
else ifeq ($(ARCH),arm64)
MODULE_SRCS += $(LOCAL_DIR)/syscalls_system_arm64.cpp
endif

# We need a header file generated by kernel/lib/vdso/rules.mk.
MODULE_COMPILEFLAGS += -I$(BUILDDIR)/kernel/lib/vdso
$(BUILDDIR)/$(LOCAL_DIR)/$(LOCAL_DIR)/syscalls.cpp.o: \
    $(BUILDDIR)/kernel/lib/vdso/vdso-valid-sysret.h

include make/module.mk
