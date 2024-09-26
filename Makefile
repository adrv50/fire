TARGET		?= 	fire
DBGPREFIX	?=	d

TOPDIR		?= 	$(CURDIR)
BUILD		:= 	build
INCLUDE		:= 	include
SOURCE		:= 	src	\
				src/Evaluator \
				src/Parser \
				src/Sema

CC			:=	clang
CXX			:=	clang++

OPTI		?=	-O0 -D_METRO_DEBUG_
COMMON		:=	$(OPTI) -Wall -Wextra -Wno-switch $(INCLUDES)
CFLAGS		:=	$(COMMON) -std=c17
CXXFLAGS	:=	$(COMMON) -std=c++20
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

.PHONY: $(BUILD) all re clean run

all: debug

debug: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile

release: $(BUILD)
	@$(MAKE) --no-print-directory OUTPUT="$(TOPDIR)/$(TARGET)" OPTI="-O3" \
		LDFLAGS="-Wl,--gc-sections,-s" -C $(BUILD) -f $(TOPDIR)/Makefile

run: all
	@./fired test.fire

$(BUILD):
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