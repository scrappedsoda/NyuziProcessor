/*
 * shared_mem_itf.hpp - Define memory addresses and shared data structure
 *
 *      Course: SoC Design Laboratoy, 2017W
 *      Author: Edwin Willeger (1326324), Markus Kessler (1225380)
 *      Mail:   [surname].[lastname]@student.tuwien.ac.at
 * 
 */

#ifndef SRC_SHARED_MEM_ITF_HPP_
#define SRC_SHARED_MEM_ITF_HPP_

/*
 * ################################################
 * #          Current memory management           #
 * ################################################
 * 
 * heap, framebuffer, stack, code data
 *     Adress:                 Entity:
 *
 *                  +****************************+
 *                  +                            +
 *                  +     Shared Mem Database    +
 *                  +                            +
 *      0x41fffffe0 +****************************+
 *                  +                            +
 *                  +    Base of the ressources  +
 *                  +                            +
 *      0x405000000 +****************************+
 *                  +                            +
 *                  +           Heap             +
 *                  +                            +
 *      0x400600000 +*****************************
 *                  +                            +
 *                  +       Framebuffer 3        +
 *                  +                            +
 *      0x400458000 +****************************+
 *                  +                            +
 *                  +       Framebuffer 2        +
 *                  +                            +
 *      0x40032c000 +****************************+
 *                  +                            +
 *                  +       Framebuffer 1        +
 *                  +                            +
 *      0x400200000 +****************************+
 *                  +                            +
 *                  +     Stacks - grow down     +
 *                  +                            +
 *                  +*****************************
 *                  +                            +
 *                  +    Base of the program     +
 *                  +                            +
 *      0x400000000 +****************************+
 *
 */
#define PROG_ADDR   0x400000000
#define FB0_ADDR    0x400200000
#define FB1_ADDR    0x40032c000
#define FB2_ADDR    0x400458000
#define HEAP_ADDR   0x400600000
#define RES_ADDR    0x405000000
#define SMDB_ADDR   0x41fffffe0

#define ACTION_BIT_ROTATE   0
#define ACTION_BIT_LEFT     1
#define ACTION_BIT_RIGHT    2
#define ACTION_BIT_UP       3
#define ACTION_BIT_DOWN     4

/* The shared memory database is used to exchange data between the PS and PL system */
typedef struct {
    u32 program_fil_st_addr;
    u32 program_fil_size;
    u32 resource_fil_st_addr;
    u32 resource_fil_size;
    u32 frame_rate;
    /**
     * bit 1: If set, rotation mode is active. Otherwise, movement mode is selected
     * bit 2: Direction left
     * bit 3: Direction right
     * bit 4: Direction up
     * bit 5: Direction down
     */
    u32 camera_actions;
} smdb_t;

#endif /* SRC_SHARED_MEM_ITF_HPP_ */
