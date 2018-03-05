OUTPUT = ./build
TARGET = tlsproxy

# Objects, relative to OUTPUT
OBJS = tlsproxy.o crypto-gnutls.o buffer.o

DEBUG = -g -O0
CFLAGS = -Wall -Werror -c $(DEBUG) -I.
LFLAGS = -Wall -Werror $(DEBUG)
LIBS = -lgnutls
CC = gcc

# Work out the build directory variants
BUILDOBJECTS := $(patsubst %o, $(OUTPUT)/%o, $(OBJS))

# Get the list of output directories
BUILDDIRS := $(sort $(foreach dir,$(BUILDOBJECTS),$(shell dirname $(dir))))

# First target for bare "make"
all: $(OUTPUT)/$(TARGET)

clean:
	@/bin/rm -rf $(OUTPUT)/*

$(OUTPUT)/$(TARGET): $(BUILDOBJECTS)
	@mkdir -p $(OUTPUT)
	$(CC) $(LFLAGS) ${BUILDOBJECTS} -o $(OUTPUT)/$(TARGET) $(LIBS)

# automatic dependencies - pull in dependency info for *existing* .o files
-include $(BUILDOBJECTS:.o=.d)

# autogenerate dependencies after a successful compilation
$(OUTPUT)/%.o: %.c
	@mkdir -p $(BUILDDIRS)
	$(CC) -c $(CFLAGS) -MMD -MP -MF"$(OUTPUT)/$*.d" -o $@ $<
	@mv -f $(OUTPUT)/$*.d $(OUTPUT)/$*.d.tmp
	@sed -e 's|.*:|$(OUTPUT)/$*.o:|' < $(OUTPUT)/$*.d.tmp > $(OUTPUT)/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(OUTPUT)/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(OUTPUT)/$*.d
	@rm -f $(OUTPUT)/$*.d.tmp

.PHONY: all clean
