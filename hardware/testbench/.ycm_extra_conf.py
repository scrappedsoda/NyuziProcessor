def Settings(**kwargs):
    return {
        "flags": [
            # "-I/usr/include/SDL2",
            "-I../generated",
            "-I/usr/local/share/verilator/include",
            "-DSDL=1",
            "-g -Wall -D_REENTRANT",
            "-I/usr/include/SDL2",
            "-lSDL2",
            "-I/usr/include/opencv4",
            "-lopencv",
            # "-g",
            # "-lSDL2",
        ],
    }
