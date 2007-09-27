/* $Id$
 * This is free software distributed under the GNU
 * General Public Licence Version 2
 * idtool.c -- Nathan Laredo <laredo@gnu.org>
 */

#define VERSION		"1.7"
#ifdef WIN32

#include <windows.h>
/* define some C99 types for windows */
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int8 uint8_t;

#else
#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#endif

#include <stdio.h>

static char messagestring[65536];
static uint32_t tm00info[32][4], tm8kinfo[32][4];
static uint32_t tm86info[32][4], tmc0info[32][4];
static uint32_t tmbainfo[32][4], allzero[4], mystery[4];

#ifdef __linux__
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <asm/unistd.h>

_syscall3(int, sched_setaffinity, pid_t, pid, size_t, len,
		__const cpu_set_t *, mask)
_syscall3(int, sched_getaffinity, pid_t, pid, size_t, len,
		cpu_set_t *, mask)
void set_cpu(int cpu)
{
	cpu_set_t	cpu_set;

	/*
	 * set up affinity so that we are locked on one cpu
	 */
		fprintf(stderr, "CPU %d: ", cpu);
	CPU_ZERO(&cpu_set);
	CPU_SET(cpu, &cpu_set);
	if (sched_setaffinity(0, sizeof cpu_set, &cpu_set)) {
		fprintf(stderr, "CPU %d: ", cpu);
		perror("set affinity failed");
	}
}
#else
void set_cpu(int cpu)
{
}
#endif

/* cpuid with abcd string order */
/* pre-stuff ebx and ecx with the value passed in "tunnel" */
void do_cpuid(void *dest, uint32_t level, uint32_t tunnel)
{
#ifndef __GNUC__
	_asm {
		mov ebx, tunnel
		mov ecx, tunnel
		mov eax, level
		cpuid
		mov edi, dest
		mov [edi], eax
		mov [edi + 4], ebx
		mov [edi + 8], ecx
		mov [edi + 12], edx
	}
#else
	uint32_t *dst = dest;
	asm("push %%ebx\n\t"
	    "mov %%ecx, %%ebx\n\t"
	    "cpuid\n\t"
	    "mov %%ebx, %%esi\n\t"
	    "pop %%ebx\n\t": "=a" (dst[0]), "=S" (dst[1]), "=c" (dst[2]), "=d" (dst[3])
	    : "a" (level), "r" (tunnel), "c" (tunnel));

#endif
}

/* Intel Application Note 485 - Order # 241618 */
/* http://developer.intel.com/design/xeon/applnots/241618.htm */

static char *edx1_feat[32] = {
	"fpu","vme","de","pse","tsc","msr","pae","mce",
	"cx8","apic","res10","sep","mtrr","pge", "mca","cmov",
	"pat","pse36","psn","clfsh","res20","ds", "acpi","mmx",
	"fxsr","sse","sse2","ss","htt", "tm","ia64","pbe"
};

static char *ecx1_feat[32] = {
	"sse3","res1","res2","monitor","ds-cpl","vmx","smx","eist",
	"tm2","ssse3","cid","res11","res12","cx16","xtpr","pdcm",
	"res16","res17","dca","sse4_1","sse4_2","x2apic","res22","popcnt",
	"res24","res25","res26","res27","res28","res29","res30","raz31"
};

static char *cway[16] = {
	"L2 off", "direct mapped", "2-way", "res3", "4-way", "res5",
	"8-way", "res7", "16-way", "res9", "res10", "res11", "res12",
	"res13", "res14", "fully associative"
};

static char *ctype[4] = {
	"null", "data", "code", "unified"
};

#ifdef __GNUC__
/* MSVC lacks c99 initialization */
static char *cache_decode[256] = {
[0x01]="code TLB, 4K pages, 4 ways, 32 entries",
[0x02]="code TLB, 4M pages, fully, 2 entries",
[0x03]="data TLB, 4K pages, 4 ways, 64 entries",
[0x04]="data TLB, 4M pages, 4 ways, 8 entries",
[0x05]="data TLB, 4M pages, 4 ways, 32 entries",
[0x06]="code L1 cache, 8 KB, 4 ways, 32 byte lines",
[0x08]="code L1 cache, 16 KB, 4 ways, 32 byte lines",
[0x0A]="data L1 cache, 8 KB, 2 ways, 32 byte lines",
[0x0B]="code TLB, 4M pages, 4 ways, 4 entries",
[0x0C]="data L1 cache, 16 KB, 4 ways, 32 byte lines",
[0x10]="data L1 cache, 16 KB, 4 ways, 32 byte lines (IA-64)",
[0x15]="code L1 cache, 16 KB, 4 ways, 32 byte lines (IA-64)",
[0x1A]="code and data L2 cache, 96 KB, 6 ways, 64 byte lines (IA-64)",
[0x22]="code and data L3 cache, 512 KB, 4 ways (!), 64 byte lines, dual-sectored",
[0x23]="code and data L3 cache, 1024 KB, 8 ways, 64 byte lines, dual-sectored",
[0x25]="code and data L3 cache, 2048 KB, 8 ways, 64 byte lines, dual-sectored",
[0x29]="code and data L3 cache, 4096 KB, 8 ways, 64 byte lines, dual-sectored",
[0x2C]="data L1 cache, 32 KB, 8 ways, 64 byte lines",
[0x30]="code L1 cache, 32 KB, 8 ways, 64 byte lines",
[0x39]="code and data L2 cache, 128 KB, 4 ways, 64 byte lines, sectored",
[0x3A]="code and data L2 cache, 192 KB, 6 ways, 64 byte lines, sectored",
[0x3B]="code and data L2 cache, 128 KB, 2 ways, 64 byte lines, sectored",
[0x3C]="code and data L2 cache, 256 KB, 4 ways, 64 byte lines, sectored",
[0x3D]="code and data L2 cache, 384 KB, 6 ways, 64 byte lines, sectored",
[0x3E]="code and data L2 cache, 512 KB, 4 ways, 64 byte lines, sectored",
[0x40]="no integrated L2 cache (P6 core) or L3 cache (P4 core)",
[0x41]="code and data L2 cache, 128 KB, 4 ways, 32 byte lines",
[0x42]="code and data L2 cache, 256 KB, 4 ways, 32 byte lines",
[0x43]="code and data L2 cache, 512 KB, 4 ways, 32 byte lines",
[0x44]="code and data L2 cache, 1024 KB, 4 ways, 32 byte lines",
[0x45]="code and data L2 cache, 2048 KB, 4 ways, 32 byte lines",
[0x46]="code and data L3 cache, 4096 KB, 4 ways, 64 byte lines",
[0x47]="code and data L3 cache, 8192 KB, 8 ways, 64 byte lines",
[0x48]="code and data L2 cache, 3072 KB, 12 ways, 64 byte lines",
[0x49]="code and data L3 cache, 4096 KB, 16 ways, 64 byte lines (P4) or code and data L2 cache, 4096 KB, 16 ways, 64 byte lines (Core 2)",
[0x4A]="code and data L3 cache, 6144 KB, 12 ways, 64 byte lines",
[0x4B]="code and data L3 cache, 8192 KB, 16 ways, 64 byte lines",
[0x4C]="code and data L3 cache, 12288 KB, 12 ways, 64 byte lines",
[0x4D]="code and data L3 cache, 16384 KB, 16 ways, 64 byte lines",
[0x4E]="code and data L2 cache, 6144 KB, 24 ways, 64 byte lines",
[0x50]="code TLB, 4K/4M/2M pages, fully, 64 entries",
[0x51]="code TLB, 4K/4M/2M pages, fully, 128 entries",
[0x52]="code TLB, 4K/4M/2M pages, fully, 256 entries",
[0x56]="L0 data TLB, 4M pages, 4 ways, 16 entries",
[0x57]="L0 data TLB, 4K pages, 4 ways, 16 entries",
[0x5B]="data TLB, 4K/4M pages, fully, 64 entries",
[0x5C]="data TLB, 4K/4M pages, fully, 128 entries",
[0x5D]="data TLB, 4K/4M pages, fully, 256 entries",
[0x60]="data L1 cache, 16 KB, 8 ways, 64 byte lines, sectored",
[0x66]="data L1 cache, 8 KB, 4 ways, 64 byte lines, sectored",
[0x67]="data L1 cache, 16 KB, 4 ways, 64 byte lines, sectored",
[0x68]="data L1 cache, 32 KB, 4 ways, 64 byte lines, sectored",
[0x70]="trace L1 cache, 12 KµOPs, 8 ways",
[0x71]="trace L1 cache, 16 KµOPs, 8 ways",
[0x72]="trace L1 cache, 32 KµOPs, 8 ways",
[0x73]="trace L1 cache, 64 KµOPs, 8 ways",
[0x77]="code L1 cache, 16 KB, 4 ways, 64 byte lines, sectored (IA-64)",
[0x78]="code and data L2 cache, 1024 KB, 4 ways, 64 byte lines",
[0x79]="code and data L2 cache, 128 KB, 8 ways, 64 byte lines, dual-sectored",
[0x7A]="code and data L2 cache, 256 KB, 8 ways, 64 byte lines, dual-sectored",
[0x7B]="code and data L2 cache, 512 KB, 8 ways, 64 byte lines, dual-sectored",
[0x7C]="code and data L2 cache, 1024 KB, 8 ways, 64 byte lines, dual-sectored",
[0x7D]="code and data L2 cache, 2048 KB, 8 ways, 64 byte lines",
[0x7E]="code and data L2 cache, 256 KB, 8 ways, 128 byte lines, sect. (IA-64)",
[0x7F]="code and data L2 cache, 512 KB, 2 ways, 64 byte lines",
[0x81]="code and data L2 cache, 128 KB, 8 ways, 32 byte lines",
[0x82]="code and data L2 cache, 256 KB, 8 ways, 32 byte lines",
[0x83]="code and data L2 cache, 512 KB, 8 ways, 32 byte lines",
[0x84]="code and data L2 cache, 1024 KB, 8 ways, 32 byte lines",
[0x85]="code and data L2 cache, 2048 KB, 8 ways, 32 byte lines",
[0x86]="code and data L2 cache, 512 KB, 4 ways, 64 byte lines",
[0x87]="code and data L2 cache, 1024 KB, 8 ways, 64 byte lines",
[0x88]="code and data L3 cache, 2048 KB, 4 ways, 64 byte lines (IA-64)",
[0x89]="code and data L3 cache, 4096 KB, 4 ways, 64 byte lines (IA-64)",
[0x8A]="code and data L3 cache, 8192 KB, 4 ways, 64 byte lines (IA-64)",
[0x8D]="code and data L3 cache, 3072 KB, 12 ways, 128 byte lines (IA-64)",
[0x90]="code TLB, 4K...256M pages, fully, 64 entries (IA-64)",
[0x96]="data L1 TLB, 4K...256M pages, fully, 32 entries (IA-64)",
[0x9B]="data L2 TLB, 4K...256M pages, fully, 96 entries (IA-64)",
[0xB0]="code TLB, 4K pages, 4 ways, 128 entries",
[0xB1]="code TLB, 4M pages, 4 ways, 4 entries and code TLB, 2M pages, 4 ways, 8 entries",
[0xB3]="data TLB, 4K pages, 4 ways, 128 entries",
[0xB4]="data TLB, 4K pages, 4 ways, 256 entries",
[0xF0]="64 byte prefetching",
[0xF1]="128 byte prefetching",
};
#endif

/* AMD volume 3 */
static char *edx8_feat[32] = {
	"fpu","vme","de","pse","tsc","msr","pae","mce",
	"cx8","apic","res10","syscall","mtrr","pge", "mca","cmov",
	"pat","pse36","res18","mp","nx","res21", "mmxext","mmx",
	"fxsr","ffxsr","res26","tscp","res28", "lm","3dnowext","3dnow"
};

static char *ecx8_feat[32] = {
	"ahf64", "cmp", "svm", "eas", "cr8d", "lzcnt", "sse4a", "msse",
	"3dnow-", "osvw", "ibs", "sse5a", "skinit", "wdt", "res14",
	"res15", "res16", "res17", "res18", "res19", "res20", "res21",
	"res22", "res23", "res24", "res25", "res26", "res27", "res28",
	"res29", "res30", "res31"
};

static char *edx87_feat[32] = {
	"ts","fid","vid","ttp","tm","stc", "res6", "res7", "fixedtsc",
	"res9", "res10", "res11", "res12", "res13", "res14", "res15",
	"res16", "res17", "res18", "res19", "res20", "res21", "res22",
	"res23", "res24", "res25", "res26", "res27", "res28", "res29",
	"res30", "res31"
};

/* TMTA internal cpuid document */
static char *edx86_feat[32] = {
	"recovery","longrun","pti","lrti","tvm","test","lrdi","sy1",
	"sy2","tsx","tta","topc","pbe","minicms","lrtil","res15",
	"res16","res17","res18","res19","res20","res21","res22","res23",
	"res24","res25","res26","res27","res28","res29","res30","res31"
};

static char *tmta_cpu[8] = {
	"TM100 (P95B)", "TM3200 (P95W)", "TM5400 (P95F)",
	"TM5400 or TM5600 (P95F)", "TM5500 or TM5800 (P95F 0.13)",
	"TM5500 or TM5800 (P95G 0.13)", "TM5700 or TM5900", "TM5700 or TM5900"
};

static char *tmta_package[16] = {
	"unknown", "29mm x 29mm", "21mm x 21mm", "unknown3",
	"unknown4", "unknown5", "unknown6", "unknown7",
	"unknown8", "unknown9", "unknown10", "unknown11",
	"unknown12", "unknown13", "unknown14", "unknown15"
};

static int output_level(uint32_t i, uint32_t *level)
{
	if (memcmp(level, allzero, 16) == 0)
		return 0;		/* nothing to show */

	sprintf(&messagestring[strlen(messagestring)], "0x%08x: "
		"eax=0x%08x ebx=0x%08x ecx=0x%08x edx=0x%08x\n",
		i, level[0], level[1], level[2], level[3]);
	return 1;			/* something to decode */
}

static void output_bits(uint32_t bits, char **names)
{
	int i, col;

	strcat(messagestring, "\t");
	for (i = col = 0; i < 32; i++) {
		if (bits & (1 << i)) {
			col += sprintf(&messagestring[strlen(messagestring)],
					"%s ", names[i]);
		}
		if (col > 60 && i < 31) {
			strcat(messagestring, "\n\t");
			col = 0;
		}
	}
	strcat(messagestring, "\n");
}

#ifdef WIN32
int APIENTRY WinMain(HINSTANCE hCurrentInst, HINSTANCE hPreviousInst,
		     LPSTR lpszCmdLine, int nCmdShow)
#else
int main(int argc, char **argv)
#endif
{
	uint32_t i, f, max, do_tmta, family;


	if (argc > 1)
	  set_cpu(strtoul(argv[1], NULL, 0));
	for (i = 0; i < 32; i++) {
		do_cpuid(tm00info[i], i, 0);
		do_cpuid(tm8kinfo[i], 0x80000000 + i, 0);
		do_cpuid(tm86info[i], 0x80860000 + i, 0);
		do_cpuid(tmc0info[i], 0xc0000000 + i, 0);
		do_cpuid(tmbainfo[i], 0xbabe0000 + i, 0);
	}
	do_cpuid(mystery, 0x8fffffff, 0);
	/* filter out bogus intel replicated levels */
	max = tm00info[0][0] & 0x1f;
	for (i = f = 0; i < 32; i++) {
		if (i > max &&
		    memcmp(tm00info[max], tm00info[i], 16) == 0)
			memset(tm00info[i], 0, 16), f++;
		if (memcmp(tm00info[max], tm8kinfo[i], 16) == 0)
			memset(tm8kinfo[i], 0, 16), f++;
		if (memcmp(tm00info[max], tm86info[i], 16) == 0)
			memset(tm86info[i], 0, 16), f++;
		if (memcmp(tm00info[max], tmc0info[i], 16) == 0)
			memset(tmc0info[i], 0, 16), f++;
		if (memcmp(tm00info[max], tmbainfo[i], 16) == 0)
			memset(tmbainfo[i], 0, 16), f++;
		if (memcmp(tm00info[max], mystery, 16) == 0)
			memset(mystery, 0, 16), f++;
	}

	do_tmta = (tm86info[0][1] == 0x6e617254 && 	/* TransmetaCPU */
	    tm86info[0][2] == 0x55504361 && tm86info[0][3] == 0x74656d73);
	family = (tm00info[1][0] & 0xf00) >> 8;

	if (tm00info[1][3] & (1 << 18)) {
		sprintf(messagestring, "PSN=%08x-%08x-%08x-%08x\n\n",
			tm00info[3][0], tm00info[3][1],
			tm00info[3][2], tm00info[3][3]);
	} else {
		sprintf(messagestring,
			"PSN=DISABLED-DISABLED-DISABLED-DISABLED\n\n");
	}

	if (f) 
		sprintf(&messagestring[strlen(messagestring)],
			"%d levels ignored\n", f);

	/* level 0 */
	output_level(0, tm00info[0]);
	sprintf(&messagestring[strlen(messagestring)],
		"\tMaximum standard function = %d, Vendor ID = ",
		tm00info[0][0]);
	strncat(messagestring, (char *) &tm00info[0][1], 4);
	strncat(messagestring, (char *) &tm00info[0][3], 4);
	strncat(messagestring, (char *) &tm00info[0][2], 4);
	strcat(messagestring, "\n");

	/* level 1 */
	output_level(1, tm00info[1]);
	sprintf(&messagestring[strlen(messagestring)],
		"\tFamily = %d, Model = %d, Stepping = %d",
		family + ((tm00info[1][0] >> 20) & 0xff),
		((tm00info[1][0] >> 12) & 0xf0)|((tm00info[1][0] >> 4) & 15),
		(tm00info[1][0] & 0xf));
	if (do_tmta) {
		sprintf(&messagestring[strlen(messagestring)],
			" (%s)", tm00info[1][0] < 0x542 ? "TM100" :
			(family == 5) ?  "Crusoe" : "Efficeon");
	}
	strcat(messagestring, "\n");
	if (family == 15 || ((tm00info[1][0] & 0xff0) == 0x6f0)) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tAPIC ID = %d, Virtual CPUs = %d, "
			"CLFUSH size = %d, Brand ID = 0x%02x\n",
			(tm00info[1][1] >> 24) & 0xff,
			(tm00info[1][1] >> 16) & 0xff,
			(tm00info[1][1] >> 8) & 0xff,
			tm00info[1][1] & 0xff);
	}
	output_bits(tm00info[1][3], edx1_feat);
	if (tm00info[1][2])	/* ecx feature flags in use? */
		output_bits(tm00info[1][2], ecx1_feat);

	if (output_level(2, tm00info[2])) {
		int byte, c;
		for (i = 0; i < 4; i++) {
		   if (tm00info[2][i] & (1 << 31))
			   continue;	/* not 8-bit descriptors */
		   for (byte = 0; byte < 4; byte++) {
			c = (tm00info[2][i] >> (byte << 3)) & 0xff;
			if (c == 0 || (byte == 0 && i == 0))
				continue;
			sprintf(&messagestring[strlen(messagestring)],
				"\t[%02x] %s\n", c,
#ifdef __GNUC__
				cache_decode[c]
#else
				"msvc lacks c99 [index]=\"value\" array init"
#endif
				);
		   }
		}
	}
	output_level(3, tm00info[3]);
	if (output_level(4, tm00info[4])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tCache = Level %d %s, "
			"%s%s\n\tthreads sharing this cache = %d\n"
			"\tprocessor cores on this die = %d\n"
			"\tways = %d, line partitions = %d, "
			"line size = %d, sets = %d\n",
			(tm00info[4][0] >> 5) & 7,
			ctype[tm00info[4][0] & 3],
			(tm00info[4][0] & 0x100) ? "Self Initializing, " : "",
			(tm00info[4][0] & 0x200) ? "Fully Associative, " : "",
			1 + ((tm00info[4][0] >> 14) & 0xfff),
			1 + ((tm00info[4][0] >> 26) & 0x3f),
			1 + ((tm00info[4][1] >> 22) & 0x3ff),
			1 + ((tm00info[4][1] >> 12) & 0x3ff),
			1 + (tm00info[4][1] & 0xfff), 1 + tm00info[4][2]);
	}
	if (output_level(5, tm00info[5])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tSmallest monitor line size = %d bytes,\n"
			"\tLargest monitor line size = %d bytes\n",
			tm00info[5][0] & 65535,
			tm00info[5][1] & 65535);
	}
	if (output_level(6, tm00info[6])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tDigital Thermometer = %d, "
			"Operating Point Protection = %d\n"
			"\tProgrammable Temp Thresholds = %d\n",
			tm00info[6][0] & 1,
			(tm00info[6][0] >> 2) & 1,
			tm00info[6][1] & 15);
	}
	/* level 7 - 31 */
	for (i = 7; i < 32; i++) {
		output_level(i, tm00info[i]);
	}

	/* level 0x80000000 */
	if (output_level(0x80000000, tm8kinfo[0])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tMaximum extended function = %d, "
			"Extended Vendor = ",
			tm8kinfo[0][0] & 0x1f);
		strncat(messagestring, (char *) &tm8kinfo[0][1], 4);
		strncat(messagestring, (char *) &tm8kinfo[0][3], 4);
		strncat(messagestring, (char *) &tm8kinfo[0][2], 4);
		if (!tm8kinfo[0][1])	/* ie. Intel Xeon */
			strcat(messagestring, "(null)");
		strcat(messagestring, "\n");
	}
	/* level 0x80000001 */
	if (output_level(0x80000001, tm8kinfo[1])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tHigh Family = %d, Model = %d, Stepping = %d\n",
			((tm8kinfo[1][0] & 0xf00) >> 8) +
			((tm8kinfo[1][0] >> 20) & 0xff),
			((tm8kinfo[1][0] >> 12) & 0xf0)|
			((tm8kinfo[1][0] >> 4) & 15),
			(tm8kinfo[1][0] & 0xf));
		output_bits(tm8kinfo[1][3], edx8_feat);
		if (tm8kinfo[1][2])
			output_bits(tm8kinfo[1][2], ecx8_feat);
	}
	/* levels 0x80000002 - 4 */
	if (output_level(0x80000002, tm8kinfo[2])) {
		output_level(0x80000003, tm8kinfo[3]);
		output_level(0x80000004, tm8kinfo[4]);
		strcat(messagestring, "\t");
		for (i = 2; i < 5; i++) {
			strncat(messagestring,
				(char *) &tm8kinfo[i], 16);
		}
		strcat(messagestring, "\n");
	}

	/* level 0x80000005 */
	if (output_level(0x80000005, tm8kinfo[5])) {
		if (tm8kinfo[5][0] & 0xffff0000) {
		    sprintf(&messagestring[strlen(messagestring)],
			"\tdata TLB 2MB/4MB Pages: %d-way, %d entries\n"
			"\tcode TLB 2MB/4MB Pages: %d-way, %d entries\n",
			(tm8kinfo[5][0] >> 24) & 0xff,
			(tm8kinfo[5][0] >> 16) & 0xff,
			(tm8kinfo[5][0] >> 8) & 0xff,
			tm8kinfo[5][0] & 0xff);
		}
		if (tm8kinfo[5][1] & 0xffff0000) {
		    sprintf(&messagestring[strlen(messagestring)],
			"\tdata TLB 4-Kbyte Pages: %d-way, %d entries\n"
			"\tcode TLB 4-Kbyte Pages: %d-way, %d entries\n",
			(tm8kinfo[5][1] >> 24) & 0xff,
			(tm8kinfo[5][1] >> 16) & 0xff,
			(tm8kinfo[5][1] >> 8) & 0xff,
			tm8kinfo[5][1] & 0xff);
		}
		sprintf(&messagestring[strlen(messagestring)],
			"\tL1 Data Cache: %d KB, %d-way, %d bytes per line,"
		        " %d per tag\n"
			"\tL1 Code Cache: %d KB, %d-way, %d bytes per line,"
		        " %d per tag\n",
			(tm8kinfo[5][2] >> 24) & 0xff,
			(tm8kinfo[5][2] >> 16) & 0xff,
			tm8kinfo[5][2] & 0xff,
			(tm8kinfo[5][2] >> 8) & 0xff,
			(tm8kinfo[5][3] >> 24) & 0xff,
			(tm8kinfo[5][3] >> 16) & 0xff,
			tm8kinfo[5][3] & 0xff,
			(tm8kinfo[5][3] >> 8) & 0xff);
	}

	/* level 0x80000006 */
	if (output_level(0x80000006, tm8kinfo[6])) {
		if (tm8kinfo[6][0] & 0xffff0000) {
		    sprintf(&messagestring[strlen(messagestring)],
			"\tL2 data TLB 2MB/4MB Pages: %s, %d entries\n"
			"\tL2 code TLB 2MB/4MB Pages: %s, %d entries\n",
			cway[(tm8kinfo[6][0] >> 28) & 0xf],
			(tm8kinfo[6][0] >> 16) & 0xfff,
			cway[(tm8kinfo[6][0] >> 12) & 0xf],
			tm8kinfo[6][0] & 0xfff);
		}
		if (tm8kinfo[6][1] & 0xffff0000) {
		    sprintf(&messagestring[strlen(messagestring)],
			"\tL2 data TLB 4-Kbyte Pages: %s, %d entries\n"
			"\tL2 code TLB 4-Kbyte Pages: %s, %d entries\n",
			cway[(tm8kinfo[6][1] >> 28) & 0xf],
			(tm8kinfo[6][1] >> 16) & 0xfff,
			cway[(tm8kinfo[6][1] >> 12) & 0xf],
			tm8kinfo[6][1] & 0xfff);
		}
		sprintf(&messagestring[strlen(messagestring)],
			"\tL2 Data Cache: %d KB, %s, %d bytes per line,"
		        " %d per tag\n",
			(tm8kinfo[6][2] >> 16) & 0xffff,
			cway[(tm8kinfo[6][2] >> 12) & 0xf],
			tm8kinfo[6][2] & 0xff,
			(tm8kinfo[6][2] >> 8) & 0xf);
	}
	/* level 0x80000007 */
	if (output_level(0x80000007, tm8kinfo[7])) {
		output_bits(tm8kinfo[7][3], edx87_feat);
	}

	/* level 0x80000008 */
	if (output_level(0x80000008, tm8kinfo[8])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tvirtual address size = %d, physical size = %d\n"
			"\tcores per die = %d, maximum cores = %d\n",
			(tm8kinfo[8][0] >> 8) & 0xff,
			tm8kinfo[8][0] & 0xff,
			(tm8kinfo[8][2] & 0xff) + 1,
			(tm8kinfo[8][2] & 0xf000) ?
			1 << ((tm8kinfo[8][2] >> 12) & 15) :
			(tm8kinfo[8][2] & 0xff) + 1);
	}
	output_level(0x80000009, tm8kinfo[9]);
	if (output_level(0x8000000a, tm8kinfo[10])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tSVM Revision = %d, number of ASIDs = %d\n",
			tm8kinfo[10][0] & 0xff, tm8kinfo[10][1]);
	}
	/* levels 0x8000000b - 31 */
	for (i = 11; i < 32; i++) {
		output_level(i + 0x80000000, tm8kinfo[i]);
	}

	/* level 0x80860000 */
	if (output_level(0x80860000, tm86info[0])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tMaximum 8086 function = %d, "
			"8086 Vendor = ",
			tm86info[0][0] & 0x1f);
		strncat(messagestring, (char *) &tm86info[0][1], 4);
		strncat(messagestring, (char *) &tm86info[0][3], 4);
		strncat(messagestring, (char *) &tm86info[0][2], 4);
		strcat(messagestring, "\n");
	}
	/* level 0x80860001 */
	if (output_level(0x80860001, tm86info[1])) {
		int procid = tm86info[1][1];
		sprintf(&messagestring[strlen(messagestring)],
			"\t8086 Family = %d, Model = %d, Stepping = %d\n",
			(tm86info[1][0] & 0xf00) >> 8,
			(tm86info[1][0] & 0xf0) >> 4,
			(tm86info[1][0] & 0xf));
		if ((procid >> 16) < 0x200) {
			sprintf(&messagestring[strlen(messagestring)],
				"\t%s %d.%d\n", !procid ? "P95 A0" :
				procid == 0x1000000 ? "P95 B0" :
				(procid & ~0xff) == 0x1000100 ? "P95 B1" :
				tmta_cpu[(procid >> 16) & 0x7],
				(procid >> 8) & 0xff, procid & 0xff);
		}
		sprintf(&messagestring[strlen(messagestring)],
			"\tNominal Frequency = %d MHz\n",
			tm86info[1][2]);
		output_bits(tm86info[1][3], edx86_feat);
	}
	if (output_level(0x80860002, tm86info[2])) {
		if (tm86info[1][1] == 0x02000000) {
			sprintf(&messagestring[strlen(messagestring)],
				"\tEfficeon %d.%d, %s package\n",
				(tm86info[2][0] >> 29) & 7,
				(tm86info[2][0] >> 25) & 15,
				tmta_package[(tm86info[2][0] >> 8) & 15]);
		}
		sprintf(&messagestring[strlen(messagestring)],
			"\tCMS Revision %d.%d.%d-%d-%d\n",
			(tm86info[2][1] >> 24) & 0xff,
			(tm86info[2][1] >> 16) & 0xff,
			(tm86info[2][1] >> 8) & 0xff,
			tm86info[2][1] & 0xff, tm86info[2][2]);
	}
	/* levels 0x80860003 - 6 */
	if (output_level(0x80860003, tm86info[3])) {
		output_level(0x80860004, tm86info[4]);
		output_level(0x80860005, tm86info[5]);
		output_level(0x80860006, tm86info[6]);
		strcat(messagestring, "\t");
		for (i = 3; i < 7; i++) {
			strncat(messagestring,
				(char *) &tm86info[i], 16);
		}
		strcat(messagestring, "\n");
	}
	/* level 0x80860007 */
	if (output_level(0x80860007, tm86info[7])) {
		sprintf(&messagestring[strlen(messagestring)],
			"\tCurrent Frequency = %d MHz @ %dmV, %d%%\n",
			tm86info[7][0], tm86info[7][1], tm86info[7][2]);
	}
	/* levels 0x80860008 - 32 */
	for (i = 8; i < 32; i++) {
		output_level(i + 0x80860000, tm86info[i]);
	}
	for (i = 0; i < 32; i++) {
		output_level(i + 0xc0000000, tmc0info[i]);
	}
	for (i = 0; i < 32; i++) {
		output_level(i + 0xbabe0000, tmbainfo[i]);
	}
	if (output_level(0x8fffffff, mystery)) {
		strcat(messagestring, "\t");
		strncat(messagestring, (char *) &mystery, 16);
		strcat(messagestring, "\n");
	}
#ifdef WIN32
	MessageBox(NULL, messagestring, "idtool "VERSION" - AMD/Intel/Transmeta CPUID VIEWER",
		   MB_OK | MB_ICONINFORMATION);
#else
	fprintf(stderr, "idtool "VERSION" - AMD/Intel/Transmeta CPUID VIEWER\n\n%s\n",
			messagestring);
#endif
	return 0;
}

