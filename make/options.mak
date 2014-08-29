###############################################################################
## Copyright(C) 2014-2024 Xundao technology Co., Ltd
##
## 功    能: 此模块用于添加功能宏
##         	 通过switch.mak中开关的设置，在此处增加需要加载的宏
## 注意事项: 添加相关编译宏时, 请使用统一的风格.
## 作    者: # Qifeng.zou # 2014.08.28 #
###############################################################################
# 调试相关宏
ifeq (__ON__, $(strip $(CONFIG_DEBUG_SUPPORT)))
	OPTIONS += __XDT_DEBUG__
endif

#日志相关宏
ifeq (__ON__, $(strip $(CONFIG_LOG_SUPPORT)))
	OPTIONS += __XDT_LOG__
	OPTIONS += __XDT_LOG_DEBUG__
endif
