##  Process this file with automake to create Makefile.in

includedir =
pkgconfigdir =

BUILD_DATE=$(shell date +%F)


bin_PROGRAMS = cwx_mq_fetch
cwx_mq_fetch_CPPFLAGS = -D_BUILD_DATE="$(BUILD_DATE)" -W -Wall  -Wno-deprecated
cwx_mq_fetch_SOURCES = \
	../common/CwxMqPoco.cpp  ../common/CwxMqTss.cpp\
	CwxMqFetchApp.cpp  CwxMqFetchConfig.cpp  cwx_mq_fetch.cpp ../common/CwxMqDef.cpp
cwx_mq_fetch_LDADD=
cwx_mq_fetch_LDFLAGS =-L@xml_expat_dir@/lib -L@cwinux_dir@/lib 
LIBS=-lcwxapp -lcwxnet -lcwxcmn  -lpthread -ldl -lrt -lz
INCLUDES=-I@cwinux_dir@/include/app -I@cwinux_dir@/include/cmn -I@cwinux_dir@/include/net -I../common

## Clean up template repositories, etc.
clean-local:
	-rm -f *~ *.bak *.rpo *.sym lib*.*_pure_* core core.*
