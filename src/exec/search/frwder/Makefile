###############################################################################
## Copyright(C) 2014-2024 Qiware technology Co., Ltd
##
## 文件名: Makefile
## 版本号: 1.0
## 描  述: 倒排服务模块
## 作  者: # Qifeng.zou # 2015.03.06 #
###############################################################################
include $(PROJ)/make/build.mak

INCLUDE = -I./incl \
			-I$(PROJ)/src/incl \
			-I$(PROJ)/src/incl/sdtp \
			-I$(PROJ)/src/incl/rtmq \
			-I$(PROJ)/src/incl/agent
INCLUDE += $(GLOBAL_INCLUDE)
LIBS_PATH = -L$(PROJ)/lib
LIBS = -lpthread -lcore -lsdtp -lrtmq
LIBS += $(SHARED_LIB)
LIBS_A = $(PROJ_LIB)/libcore.a $(PROJ_LIB)/librtmq.a

SRC_LIST = frwder.c \
			frwd_comm.c \
			frwd_mesg.c \
			frwd_conf.c

OBJS = $(subst .c,.o, $(SRC_LIST))
HEADS = $(call func_get_dep_head_list, $(SRC_LIST))

TARGET = frwder

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(INCLUDE) $(LIBS_PATH) $(LIBS)
	@echo "CC $@"
	@mv $@ $(PROJ_BIN)/$@-$(VERSION)
	@rm -f $(PROJ_BIN)/$@
	@ln -s $(PROJ_BIN)/$@-$(VERSION) $(PROJ_BIN)/$@
	@echo "$@ is OK!"

$(OBJS): %.o : %.c $(HEADS)
	@$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)
	@echo "CC $(PWD)/$<"

clean:
	@rm -fr *.o $(PROJ_BIN)/$(TARGET)
	@rm -fr *.o $(PROJ_BIN)/$(TARGET)-$(VERSION)
	@echo "rm -fr *.o $(PROJ_BIN)/$(TARGET)"
