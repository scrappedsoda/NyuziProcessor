import re
import sys
import ctypes
import os
import util
import struct
		
# Check if parameters are correct
if len(sys.argv) != 3:
	print("usage: python " + sys.argv[0] + " <input_file_name> <output_file_name")
	sys.exit(1)	

# Get file base name (without file extension) from arguments
hex_file_name = sys.argv[1]

# Get start address from arguments
bin_file_name = sys.argv[2]
	
# Open files
fobj_in = open(hex_file_name)
fobj_out = open(bin_file_name,"wb")

# Parse file line by line
for line in fobj_in:
	hex_val = util.s2h(line)
	fobj_out.write(struct.pack('>I',hex_val))
	
# Everything went well
print("Success")
	
	
