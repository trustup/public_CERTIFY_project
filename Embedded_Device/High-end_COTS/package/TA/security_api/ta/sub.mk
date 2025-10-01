global-incdirs-y += include

# global-incdirs-y += lib/p-abc-main/include
# global-incdirs-y += lib/p-abc-main/lib/pfecCwrapper/include

srcs-y += security_api_ta.c
srcs-y += utils.c

libdirs += ../

# libnames += dpabc_psms_middleware_bundled
# libdeps += ../libdpabc_psms_middleware_bundled.a

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
