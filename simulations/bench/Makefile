TARGET=simtop
ADDTARGET=app_add_top

.PHONY: $(TARGET) $(ADDTARGET)

RTLSRC=../src
all: $(TARGET) $(ADDTARGET)

LDFLAGS=
CFLAGS=-g -O3

$(TARGET):
	verilator -Wno-fatal -I$(RTLSRC)/ --cc $(@) --trace --exe ../$(@).cpp  -Mdir $(@) -CFLAGS "$(CFLAGS)"
	make -C $(@) -f V$(@).mk

run: $(TARGET)
	./$(TARGET)/V$(TARGET)


$(ADDTARGET):
	verilator -Wno-fatal -I$(RTLSRC)/ --cc $(@) --trace --exe ../$(@).cpp  -Mdir $(@) -CFLAGS "$(CFLAGS)"
	make -C $(@) -f V$(@).mk

run_add: $(ADDTARGET)
	./$(ADDTARGET)/V$(ADDTARGET)
ra: run_add



view:
	gtkwave ./$(TARGET)/V$(TARGET).vcd &

clean:
	rm -rf *.o $(TARGET)
	rm -rf *.o $(ADDTARGET)

distclean: clean
	rm -rf *.txt *.vcd *.mif *.orig
