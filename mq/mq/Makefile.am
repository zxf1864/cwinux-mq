##  Process this file with automake to create Makefile.in

includedir =
pkgconfigdir =

BUILD_DATE=$(shell date +%F)


bin_PROGRAMS = cwx_mq
cwx_mq_CPPFLAGS = -D_BUILD_DATE="$(BUILD_DATE)" -W -Wall  -Wno-deprecated
cwx_mq_SOURCES = CwxMqApp.cpp\
		CwxMqQueueHandler.cpp\
		CwxMqConfig.cpp\
		CwxMqMasterHandler.cpp\
		CwxMqDispHandler.cpp\
		CwxMqRecvHandler.cpp\
		cwx_mq.cpp\
		../common/CwxMqDef.cpp\
                ../common/CwxMqPoco.cpp\
		../common/CwxMqQueueMgr.cpp\
		../common/CwxSidLogFile.cpp\
		../common/CwxMqTss.cpp\
                ../common/CwxBinLogMgr.cpp\
                ../common/CwxMqConnector.cpp
cwx_mq_LDADD=
cwx_mq_LDFLAGS =-L@cwinux_dir@/lib
LIBS=-lcwxapp -lcwxnet -lcwxcmn -lpthread -ldl -lrt -lz
INCLUDES=-I@cwinux_dir@/include/app -I@cwinux_dir@/include/cmn -I@cwinux_dir@/include/net  -I../common

## Clean up template repositories, etc.
clean-local:
	-rm -f *~ *.bak *.rpo *.sym lib*.*_pure_* core core.*
