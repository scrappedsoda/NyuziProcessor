TARGET=simtop
ADDTARGET=app_add_top
MULTARGET=app_mul_top

.PHONY: $(TARGET) $(ADDTARGET) $(MULTARGET)

RTLSRC=../src
all: $(TARGET) $(ADDTARGET)

LDFLAGS=
CFLAGS=-g -O3

COMHDL=hdl
#OBJDIR=obj

$(TARGET):
	verilator -Wno-fatal -I$(RTLSRC)/ --cc $(@) --trace --exe ../$(@).cpp  -Mdir $(COMHDL) -CFLAGS "$(CFLAGS)"
	make -C $(COMHDL) -f V$(@).mk

run: $(TARGET)
	./$(COMHDL)/V$(TARGET)

#-------------------------------------------------------------------------------

$(ADDTARGET):
	verilator -Wno-fatal -I$(RTLSRC)/ --cc $(@) --trace --exe ../$(@).cpp  -Mdir $(COMHDL) -CFLAGS "$(CFLAGS)"
	make -C $(COMHDL) -f V$(@).mk

run_add: $(ADDTARGET)
	./$(COMHDL)/V$(ADDTARGET)
ra: run_add

#-------------------------------------------------------------------------------

$(MULTARGET):
	verilator -Wno-fatal -I$(RTLSRC)/ --cc $(@) --trace --exe ../$(@).cpp  -Mdir $(COMHDL) -CFLAGS "$(CFLAGS)"
	make -C $(COMHDL) -f V$(@).mk

run_mul: $(MULTARGET)
	./$(COMHDL)/V$(MULTARGET)
rm: run_mul

#-------------------------------------------------------------------------------


view:
	gtkwave ./$(TARGET)/V$(TARGET).vcd &

clean:
	rm -rf *.o $(TARGET)
	rm -rf *.o $(ADDTARGET)

distclean: clean
	rm -rf *.txt *.vcd *.mif *.orig
