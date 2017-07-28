ARCH           = x86_64              
CFLAGS         = -O2 -Wall -Werror -g -lrt
LDFLAGS        = -m64                 
LDLIBS         = -L${LIBLITMUS} -llitmus -pthread -lm
CPPFLAGS       = -D_XOPEN_SOURCE=600 -D_GNU_SOURCE -m64 -DARCH=x86_64 -I${LIBLITMUS}/include -I${LIBLITMUS}/arch/x86/include -I${LIBLITMUS}/arch/x86/include/uapi -I${LIBLITMUS}/arch/x86/include/generated/uapi
CC             = /usr/bin/gcc        
LD             = /usr/bin/ld         
AR             = /usr/bin/ar         
