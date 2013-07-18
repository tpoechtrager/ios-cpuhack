/*
 * Hack iOS binaries to a different cpu subtype.
 * You are doing this at your own risk.
 *
 * Copyright (C) 2013 By Thomas Poechtrager
 * t.poechtrager@gmail.com
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

/*
 * mach_header struct is taken from:
 * https://developer.apple.com/library/mac/#documentation/DeveloperTools/Conceptual/MachORuntime/Reference/reference.html
 * enums are taken from the LLVM documentation:
 * http://llvm.org/docs/doxygen/html/MachOFormat_8h_source.html
 */

static const uint32_t MH_MAGIC = 0xFEEDFACE;

typedef struct
{
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
} mach_header;

enum
{
    CTFM_ArchMask =  0xFF000000,
    CTFM_ArchABI64 = 0x01000000
};

enum
{
    CTM_i386      = 7,
    CTM_x86_64    = CTM_i386 | CTFM_ArchABI64,
    CTM_ARM       = 12,
    CTM_SPARC     = 14,
    CTM_PowerPC   = 18,
    CTM_PowerPC64 = CTM_PowerPC | CTFM_ArchABI64
};

enum
{
    CSARM_ALL    = 0,
    CSARM_V4T    = 5,
    CSARM_V6     = 6,
    CSARM_V5TEJ  = 7,
    CSARM_XSCALE = 8,
    CSARM_V7     = 9,
    CSARM_V7F    = 10,
    CSARM_V7S    = 11,
    CSARM_V7K    = 12,
    CSARM_V6M    = 14,
    CSARM_V7M    = 15,
    CSARM_V7EM   = 16
};

static const char *const UNKNOWN = "unknown";

static const char *getcpusubtype(uint32_t cpusubtype)
{
    switch (cpusubtype)
    {
        case CSARM_ALL:     return "armall";
        case CSARM_V4T:     return "armv4t";
        case CSARM_V6:      return "armv6";
        case CSARM_V5TEJ:   return "armv5tej";
        case CSARM_XSCALE:  return "armxscale";
        case CSARM_V7:      return "armv7";
        case CSARM_V7F:     return "armv7f";
        case CSARM_V7S:     return "armv7s";
        case CSARM_V7K:     return "armv7k";
        case CSARM_V6M:     return "armv6m";
        case CSARM_V7M:     return "armv7m";
        case CSARM_V7EM:    return "armv7em";
        default:            return UNKNOWN;
    }
}

static uint32_t getcpusubtype_i(const char *cpusubtype)
{
    if (!strcmp(cpusubtype, "armall"))    return CSARM_ALL;
    if (!strcmp(cpusubtype, "armv6"))     return CSARM_V6;
    if (!strcmp(cpusubtype, "armv5tej"))  return CSARM_V5TEJ;
    if (!strcmp(cpusubtype, "armxscale")) return CSARM_XSCALE;
    if (!strcmp(cpusubtype, "armv7"))     return CSARM_V7;
    if (!strcmp(cpusubtype, "armv7f"))    return CSARM_V7F;
    if (!strcmp(cpusubtype, "armv7s"))    return CSARM_V7S;
    if (!strcmp(cpusubtype, "armv7k"))    return CSARM_V7K;
    if (!strcmp(cpusubtype, "armv6m"))    return CSARM_V6M;
    if (!strcmp(cpusubtype, "armv7m"))    return CSARM_V7M;
    if (!strcmp(cpusubtype, "armv7em"))   return CSARM_V7EM;
    return -1;
}

static int isnumeric(const char *str)
{
    const char *p;
    for (p = str; *p; p++)
    {
        if (*p >= '9' || *p <= '0') return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    FILE *f;
    mach_header m;
    unsigned haveflags = 0, i;
    uint32_t newcpusubtype;

    if (argc <= 2)
    {
        printf("./ios-cpuhack <in> <new cpu subtype>\n");
        return 1;
    }

    f = fopen(argv[1], "r+b");
    newcpusubtype = getcpusubtype_i(argv[2]);
    if (newcpusubtype == (uint32_t)-1)
    {
        int isnum = isnumeric(argv[2]);
        if (isnum) newcpusubtype = strtoul(argv[2], NULL, 10);

        if (!isnum || getcpusubtype(newcpusubtype) == UNKNOWN)
        {
            printf("invalid cpu subtype given!\n");
            return 1;
        }
    }

    if (!f)
    {
        printf("opening file failed\n");
        return 1;
    }

    if (fread(&m, 1, sizeof(mach_header), f) != sizeof(mach_header))
    {
        fclose(f);
        printf("reading header failed");
        return 1;
    }

    if (m.magic != MH_MAGIC)
    {
        fclose(f);
        printf("invalid header magic\n");
        return 1;
    }

    printf("cpu type: %u  cpu subtype: %d (%s)  bit flags set: ", m.cputype, m.cpusubtype, getcpusubtype(m.cpusubtype));

    for (i = 0; i < sizeof(uint32_t)*CHAR_BIT; i++)
    {
        if (m.flags&(1<<i))
        {
            haveflags = 1;
            printf("%d, ", i);
        }
    }

    if (haveflags) printf("\b\b \n");
    else printf("-\n");

    if (m.cputype == CTM_ARM)
    {
        m.cpusubtype = newcpusubtype;

        fseek(f, 0, SEEK_SET);
        if (fwrite(&m, 1, sizeof(mach_header), f) != sizeof(mach_header))
        {
            fclose(f);
            printf("writing header failed\n");
            return 1;
        }

        printf("set cpu subtype to: %d (%s)\n", m.cpusubtype, getcpusubtype(m.cpusubtype));
    }
    else
    {
        printf("error: not an arm binary!\n");
    }

    fclose(f);
    return 0;
}
