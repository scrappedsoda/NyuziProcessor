TARGET=simtop
.PHONY: $(TARGET)
RTLSRC=../src
all: $(TARGET)

LDFLAGS=
CFLAGS=-g -O3

$(TARGET):
	verilator -Wno-fatal -I$(RTLSRC)/ --cc $(@) --trace --exe ../$(@).cpp  -Mdir $(@) -CFLAGS "$(CFLAGS)"
	make -C $(@) -f V$(@).mk

run: $(TARGET)
	./$(TARGET)/V$(TARGET)

view:
	gtkwave ./$(TARGET)/V$(TARGET).vcd &

clean:: 
	rm -rf *.o $(TARGET)
distclean:: clean
	rm -rf *~ *.txt *.vcd *.mif *.orig
