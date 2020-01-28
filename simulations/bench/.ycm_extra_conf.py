def Settings(**kwargs):
    return {
        "flags": [
            "-std=c++14",
            "-Wall",
            "-I.",
            "-I./hdl",
            "-I./simtop",
            "-I/usr/local/share/verilator/include",
            "-L.",
            "-L/usr/local/share/verilator/include",
            "-L./simtop",
            "-I./app_add_top",
            "-L./app_add_top",
        ],
    }
