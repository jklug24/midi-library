PROJDIR		:= $(realpath $(CURDIR))
SOURCEDIR	:= $(PROJDIR)/root
BUILDDIR	:= $(PROJDIR)/build

TARGET		= midiManager.exe

VERBOSE 	= TRUE

DIRS 		= src include
SOURCEDIRS 	= $(foreach dir, $(DIRS), $(addprifix $(SOURCEDIR)/, $(dir)))
TARGETDIRS	= $(foreach dir, $(DIRS), $(addprifix $(BUILDDIR)/, $(dir)))

INCLUDES	= $(foreach dir, $(SOURCEDIRS), $(addprifix -I, $(DIR)))

VPATH		= $(SOURCEDIRS)

SOURCES		= $(foreach dir, $(SOURCEDIRS), $(wildcard $(dir)/*.c))

OBJS		:= $(subst $(SOURCEDIR), $(BUILDDIR), $(SOURCES:.c=.o))

DEPS 		= $(OBJS:.o=.d)

CC 		= gcc

RM = rm -rf 
RMDIR = rm -rf 
MKDIR = mkdir -p
ERRIGNORE = 2>/dev/null
SEP=/


# Remove space after separator
PSEP = $(strip $(SEP))

# Hide or not the calls depending of VERBOSE
ifeq ($(VERBOSE),TRUE)
    HIDE =  
else
    HIDE = @
endif

# Define the function that will generate each rule
define generateRules
$(1)/%.o: %.c
    @echo Building $$@
    $(HIDE)$(CC) -c $$(INCLUDES) -o $$(subst /,$$(PSEP),$$@) $$(subst /,$$(PSEP),$$<) -MMD
endef

.PHONY: all clean directories 

all: directories $(TARGET)

$(TARGET): $(OBJS)
    $(HIDE) echo Linking $@
    $(HIDE) $(CC) $(OBJS) -o $(TARGET)

# Include dependencies
-include $(DEPS)

# Generate rules
$(foreach targetdir, $(TARGETDIRS), $(eval $(call generateRules, $(targetdir))))

directories: 
    $(HIDE)$(MKDIR) $(subst /,$(PSEP),$(TARGETDIRS)) $(ERRIGNORE)

# Remove all objects, dependencies and executable files generated during the build
clean:
    $(HIDE)$(RMDIR) $(subst /,$(PSEP),$(TARGETDIRS)) $(ERRIGNORE)
    $(HIDE)$(RM) $(TARGET) $(ERRIGNORE)
    @echo Cleaning done 



