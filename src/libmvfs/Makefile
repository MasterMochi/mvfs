#******************************************************************************#
#*                                                                            *#
#* src/libmvfs/Makefile                                                       *#
#*                                                                 2019/09/16 *#
#* Copyright (C) 2019 Mochi.                                                  *#
#*                                                                            *#
#******************************************************************************#
#******************************************************************************#
#* 設定                                                                       *#
#******************************************************************************#
# ライブラリ名
LIB = libmvfs

# ソースコード
SRCS  = Close.c
SRCS += Fd.c
SRCS += Get.c
SRCS += Mount.c
SRCS += Open.c
SRCS += Read.c
SRCS += Sched.c
SRCS += Send.c
SRCS += Select.c
SRCS += Write.c

# ビルドディレクトリ
BUILD_DIR   = ../../build
# リリースディレクトリ
RELEASE_DIR = $(BUILD_DIR)/release
# オブジェクトディレクトリ
OBJ_DIR     = $(BUILD_DIR)/obj/$(LIB)

# Cフラグ
CFLAGS  = -O
CFLAGS += -m32
CFLAGS += -Wall
CFLAGS += -masm=intel
CFLAGS += -ffreestanding
CFLAGS += -I../include
CFLAGS += -I$(RELEASE_DIR)/include
CFLAGS += -I$(BUILD_DIR)/include
ifdef BASE_CFLAGS
CFLAGS += $(BASE_CFLAGS)
endif


#******************************************************************************#
#* 定義                                                                       *#
#******************************************************************************#
# オブジェクトサブディレクトリ
OBJ_SUBDIRS = $(sort $(addprefix $(OBJ_DIR)/, $(dir $(SRCS))))

# オブジェクトファイル
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))

# 依存関係ファイル
DEPS = $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.d))


#******************************************************************************#
#* phonyターゲット                                                            *#
#******************************************************************************#
# コンパイル
.PHONY: all
all: $(OBJ_SUBDIRS) $(RELEASE_DIR)/$(LIB).a Makefile

# 全生成ファイルの削除
.PHONY: clean
clean:
	-rm -rf $(RELEASE_DIR)/$(LIB).a
	-rm -rf $(OBJ_DIR)


#******************************************************************************#
#* 生成規則                                                                   *#
#******************************************************************************#
# 依存関係
-include $(DEPS)

# オブジェクトサブディレクトリ生成
ifdef OBJ_SUBDIRS
$(OBJ_SUBDIRS):
	mkdir -p $@
endif

# ライブラリ生成
$(RELEASE_DIR)/$(LIB).a: $(OBJS) Makefile
	$(AR) rcs $(OBJ_DIR)/$(LIB).a $(OBJS)
	ln -srf $(OBJ_DIR)/$(LIB).a $@

# アセンブル
$(OBJ_DIR)/%.o: %.s Makefile
	$(AS) -o $@ $<

# コンパイル
$(OBJ_DIR)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -o $@ -c $< -MD -MP


#******************************************************************************#
