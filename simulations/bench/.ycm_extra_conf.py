def Settings(**kwargs):
    return {
        'flags':['-std=c++14','-Wall', '-I.', '-I./simtop', '-I/usr/local/share/verilator/include', '-L.', '-L/usr/local/share/verilator/include', '-L./simtop'],
        }
