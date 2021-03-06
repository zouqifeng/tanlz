###############################################################################
## Copyright(C) 2013-2014 Qiware technology Co., Ltd
##
## 文件名: Makefile
## 版本号: 1.0
## 描  述: 核心模块的编译
## 作  者: # Qifeng.zou # 2014.08.28 #
###############################################################################
include $(PROJ)/make/build.mak

INCLUDE = -I. \
		  -I$(PROJ)/src/incl
INCLUDE += $(GLOBAL_INCLUDE)
LIBS_PATH = -L$(PROJ)/lib
LIBS = -lpthread
LIBS += $(SHARED_LIB)

SRC_LIST = log.c \
			slot.c \
			lock.c \
			http.c \
			list.c \
			list2.c \
			menu.c \
			ring.c \
			queue.c \
		    logsvr.c \
			sig_queue.c \
			slab.c \
			redo.c \
			shm_slab.c \
			shm_opt.c \
			mem_ref.c \
			mem_seg_ref.c \
			mem_pool.c \
			shm_ring.c \
			shm_slot.c \
			shm_queue.c \
			shm_list.c \
			shm_hash.c \
			btree.c \
			shm_btree.c \
			rb_tree.c \
			trie.c \
			avl_tree.c \
			xml_comm.c \
			xml_print.c \
			xml_tree.c \
			thread_pool.c \
			sck_tcp.c \
			sck_udp.c \
			sck_unix.c \
			hash_tab.c \
			hash_alg.c \
			str.c \
			uri.c \
			iovec.c \
			vector.c \
			quick_sort.c

OBJS = $(subst .c,.o, $(SRC_LIST))
HEADS = $(call func_get_dep_head_list, $(SRC_LIST))

TARGET = libcore.so
TARGET_A = libcore.a

.PHONY: all clean

all: $(TARGET) $(TARGET_A)
$(TARGET): $(OBJS)
	@$(CC) $(LFLAGS) -o $@ $(OBJS) $(INCLUDE) $(LIBS_PATH) $(LIBS)
	@echo "CC $@"
	@mv $@ $(PROJ_LIB)
	@echo "$@ is OK!"

$(TARGET_A): $(OBJS)
	@$(AR) $(AFLAGS) -s $@ $(OBJS)
	@echo "AR $@"
	@mv $@ $(PROJ_LIB)

$(OBJS): %.o : %.c $(HEADS)
	@$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)
	@echo "CC $(PWD)/$<"

clean:
	@rm -fr *.o $(PROJ_LIB)/$(TARGET) $(PROJ_LIB)/$(TARGET_A)
	@echo "rm -fr *.o $(PROJ_LIB)/$(TARGET) $(PROJ_LIB)/$(TARGET_A)"
