

struct mdproc {
    int *md_regs;               /* registers on current frame */
    int md_flags;               /* machine-dependent flags */
    int md_upte[16];        /* ptes for mapping u page */
    int md_rval[2];             /* return value of syscall */
    int md_ss_addr;             /* single step address for ptrace */
    int md_ss_instr;            /* single step instruction for ptrace */
};

/* md_flags */
#define MDP_FPUSED      0x0001  /* floating point coprocessor used */
