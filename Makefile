

SRCS        := $(shell find ./ -name "*.c" )
OBJS        := $(patsubst %.c,%.o,$(SRCS))

#MOD_VER         := $(shell grep -i "__mod_ver__" ./$(MODULE).h|awk '{print $$3}'|sed 's/\"//g')
LDFLAG          := -Wall -g -pthread -lrt 
CFLAG_OBJS      := -Wall -Werror -I. -g 
TARGET          := zkc


.PHONY: all
all: $(OBJS)
	$(CC) $(LDFLAG) $(OBJS) -o $(TARGET)

$(OBJS):%.o:%.c
	$(CC) $(CFLAG_OBJS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags

