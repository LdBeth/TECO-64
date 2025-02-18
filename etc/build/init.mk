# Define defaults we'll use if not specified by command-line options to Makefile.

buffer  ?= gap
display ?= 1
int     ?= 32
paging  ?= vm

#  Edit buffer options.

ifeq (${buffer}, gap)               # Did user ask for a gap buffer?

    # Nothing to remove if gap buffer.

else ifeq (${buffer}, rope)         # Did user ask for a rope buffer?

    $(error Rope buffer handler is not yet implemented)

else                                # We don't know what the user wants

    $(error Unknown buffer handler: ${buffer})

endif

#  Display options.

ifeq (${display}, 1)

    EXCLUDES += stubs.c
    LINKOPTS += -l ncurses

else

    EXCLUDES += display.c color_cmd.c key_cmd.c map_cmd.c status.c

endif

#  Integer size options.

ifeq (${int}, 32)                  # Did user ask for 32-bit integers?

    DEFINES += -D INT_T=32
    DOXYGEN +=    INT_T=32

else ifeq (${int}, 64)              # Did user ask for 64-bit integers?

    DEFINES += -D INT_T=64
    DOXYGEN +=    INT_T=64

else                                # We don't know what the user wants

    $(error Unknown integer size: ${int}: expected 32 or 64)

endif

#  Memory paging options.

ifeq (${paging}, vm)                # Did user ask for virtual memory paging?

    EXCLUDES += page_file.c page_std.c
    DEFINES += -D PAGE_VM
    DOXYGEN += PAGE_VM

else ifeq (${paging}, std)          # Did user ask for standard TECO paging?

    EXCLUDES += page_file.c page_vm.c

else ifeq (${paging}, file)         # Did user ask for holding file paging?

    #  This is a placeholder for eventual implementation of a method for using
    #  a "holding file" to allow backwards paging, as described in The Craft of
    #  Text Editing, by Craig A. Finseth.

    $(error Holding file paging is not yet implemented)

else                                # We don't know what the user wants

    $(error Unknown paging handler: ${paging})

endif

#  Debugging and compiler optimization options.

ifdef   gdb                         # Can't build for both gdb and gprof
ifdef   gprof

    $(error gdb and gprof options cannot both be included for a build)

endif
endif

ifdef   gdb                         # Enable use of gdb

    CFLAGS   += -g -O0
    LINKOPTS += -g -O0

else ifdef gprof                    # Enable use of gprof

    CFLAGS   += -pg -O0
    LINKOPTS += -pg -O0
    DEFINES += -D PROFILE

else                                # Default if neither gdb nor gprof

	CFLAGS   += -Ofast
    LINKOPTS += -Ofast -s

endif

ifdef   debug                       # Enable debugging

    DEFINES += -D DEBUG=$(debug)
    DOXYGEN += DEBUG=$(debug)

endif

#  Options that remove features or restrictions in order to improve speed.

ifdef   inline                      # Enable inline optimizations

    DEFINES += -D INLINE
    DOXYGEN +=    INLINE

endif

ifdef   lto                         # Enable link-time optimization

    CFLAGS += -flto=auto
    LINKOPTS += -flto=auto

endif

ifdef   ndebug                      # Disable run-time assertions

    DEFINES += -D NDEBUG
    DOXYGEN +=    NDEBUG

endif

ifdef   nstrict                     # Disable strict syntax checking

    DEFINES += -D NSTRICT
    DOXYGEN +=    NSTRICT

endif

ifdef   ntrace                      # Disable command tracing

    DEFINES += -D NTRACE
    DOXYGEN +=    NTRACE

endif
