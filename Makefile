TARGET		?= 	fire
DBGPREFIX	?=	d

BUILD			:= 	build
BUILD_RELEASE	:=	build_release

TOPDIR		?= 	$(CURDIR)

INCLUDE		:= 	include
SOURCE		:= 	src \
				src/Builtins \
				src/Debug \
				src/Driver \
				src/Parser \
				src/Sema \
				src/Evaluator \
				src/Lexer \
				src/Utils

CC			:=	clang
CXX			:=	clang++

OPTI		?=	-O0 -g -Wall -Wextra -D_FIRE_DEBUG_

COMMON		:=	$(OPTI) \
				$(INCLUDES) \
				-Wno-switch \
				-Wno-unused-label \
				-Wno-unused-parameter \
				-Wno-unused-variable \
				-Wno-unused-but-set-variable

CFLAGS		:=	$(COMMON) -std=c17
CXXFLAGS	:=	$(COMMON) -std=gnu++20
LDFLAGS		:=

%.o: %.c
	@echo $(notdir $<)
	@$(CC) -MP -MMD -MF $*.d $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	@echo $(notdir $<)
	@$(CXX) -MP -MMD -MF $*.d $(CXXFLAGS) -c -o $@ $<

ifneq ($(BUILD), $(notdir $(CURDIR)))

CFILES		= $(notdir $(foreach dir,$(SOURCE),$(wildcard $(dir)/*.c)))
CXXFILES	= $(notdir $(foreach dir,$(SOURCE),$(wildcard $(dir)/*.cpp)))

export OUTPUT		= $(TOPDIR)/$(TARGET)$(DBGPREFIX)
export VPATH		= $(foreach dir,$(SOURCE),$(TOPDIR)/$(dir))
export INCLUDES		= $(foreach dir,$(INCLUDE),-I$(TOPDIR)/$(dir))
export OFILES		= $(CFILES:.c=.o) $(CXXFILES:.cpp=.o)

.PHONY: $(BUILD) $(BUILD_RELEASE) all re clean run

all: debug release

debug: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile

release: $(BUILD_RELEASE)
	@$(MAKE) --no-print-directory \
		BUILD=$(BUILD_RELEASE) \
		OUTPUT="$(TOPDIR)/$(TARGET)" \
		OPTI="-O3" \
		LDFLAGS="-Wl,--gc-sections,-s" \
		-C $(BUILD_RELEASE) -f $(TOPDIR)/Makefile

run: all
	@echo -------------------------------------
	@./fired test.fr

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

$(BUILD_RELEASE):
	@[ -d $@ ] || mkdir -p $@

clean:
	rm -rf $(TARGET) $(TARGET)$(DBGPREFIX) $(BUILD)

re: clean all

else

DEPENDS		:=	$(OFILES:.o=.d)

$(OUTPUT): $(OFILES)
	@echo linking...
	@$(CXX) -pthread $(LDFLAGS) -o $@ $^

-include $(DEPENDS)

endif