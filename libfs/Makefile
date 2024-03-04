# Target library
lib := libfs.a
target := libfs
objs := fs.o disk.o

CC := gcc
CFLAGS := -Wall -Werror -Wextra -MMD

ifneq ($(V),1)
Q = @
endif

all: $(target)
all: $(lib)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

$(target): $(objs)
	@echo "AR	$@"
	$(Q)ar rcs $(lib) $^

%.o: %.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "CLEAN"
	$(Q)rm -f $(lib) $(objs) $(deps)


## TODO: Phase 1
