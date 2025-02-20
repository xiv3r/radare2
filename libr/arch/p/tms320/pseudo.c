/* radare - LGPL - Copyright 2020-2024 - pancake */

#include <r_lib.h>
#include <r_flag.h>
#include <r_anal.h>
#include <r_asm.h>

// https://www.ti.com/lit/ug/spru732j/spru732j.pdf

static int replace(int argc, const char *argv[], char *newstr) {
	int i, j, k;
	struct {
		int narg;
		const char *op;
		const char *str;
	} ops[] = {
		{3, "add", "3 = 1 + 2" },    // add b12, b1, b9 -> b9 = b12 + b1
		{3, "addu", "3 = 1 + 2" },
		{3, "addw", "3 = 1 + 2" },
		{3, "addaw", "3 = 1 + 2" },
		{3, "addab", "3 = 1 + 2" },
		{3, "addah", "3 = 1 + 2" },
		{2, "addk", "2 += 1" },      // addk 123, b0 -> b0 += 123
		{3, "sadd", "3 = 1 + 2" },   // sadd b12, b1, b9 -> b9 = b12 + b1
		{3, "sadd2", "3 = 1 + 2" },  // sadd2 b12, b1, b9 -> b9 = b12 + b1
		{3, "sub", "3 = 1 - 2" },    // sub b12, b1, b9 -> b9 = b12 - b1
		{3, "subu", "3 = 1 - 2" },   // sub b12, b1, b9 -> b9 = b12 - b1
		{3, "sub2", "3 = 1 - 2" },   // sub b12, b1, b9 -> b9 = b12 - b1
		{3, "subab", "3 = 1 - 2" },  // sub b12, b1, b9 -> b9 = b12 - b1
		{3, "ssub", "3 = 1 - 2" },   // ssub b12, b1, b9 -> b9 = b12 - b1
		{2, "mv", "2 = 1" },
		{2, "mvk", "2 = 1" },        // mvk 1, a0 -> a0 = 1
		{2, "mvklh", "2 = (half) 1" },// mvk 1, a0 -> a0 = 1
		{3, "band", "3 = 1 & 2" },   //
		{1, "zero", "1 = zero" },
		{3, "andn", "4 = 1 ~ 2" }, //
		{3, "cmpgtu", "3 = 1 cmpgtu 2" }, //
		{3, "cmpeq", "3 = 1 == 2" }, //
		{3, "cmpge", "3 = 1 >= 2" }, //
		{3, "cmplt", "3 = 1 <= 2" }, //
		{3, "smpylh", "3 = 1 * 2" }, //
		{3, "smpy", "3 = 1 * 2" }, //
		{3, "smpyh", "3 = 1 * 2" }, //
		{3, "mpyu4", "3 = 1 * 2" }, //
		{3, "avg2", "3 = 1 avg 2" }, //
		{3, "pack2", "3 = 1 pack 2" }, //
		{3, "smpy", "3 = 1 * 2" }, //
		{3, "max2", "3 = max(1, 2)" }, //
		{3, "mpy", "3 = 1 * 2" }, //
		{3, "mpy2", "3 = 1 * 2" }, //
		{3, "mpyu", "3 = 1 * 2" }, //
		{3, "mpyh", "3 = 1 * 2" }, //
		{3, "mpyhl", "3 = 1 * 2" }, //
		{3, "mpyhl", "3 = 1 * 2" }, //
		{3, "mpylh", "3 = 1 * 2" }, //
		{3, "mpysu", "3 = 1 * 2" }, //
		{3, "smpyhl", "3 = 1 * 2" }, //
		{3, "mpyhlu", "3 = 1 * 2" }, //
		{3, "mpyhslu", "3 = 1 * 2" }, //
		{3, "mpyluhs", "3 = 1 * 2" }, //
		{3, "mpyhi", "3 = 1 * 2" }, //
		{3, "mpyhu", "3 = 1 * 2" }, //
		{3, "mpyhus", "3 = 1 * 2" }, //
		{3, "mpyhsu", "3 = 1 * 2" }, //
		{3, "mpyhul", "3 = 1 * 2" }, //
		{3, "mpyhuls", "3 = 1 * 2" }, //
		{3, "mpyhir", "3 = 1 * 2" }, //
		{3, "mpyli", "3 = 1 * 2" }, //
		{3, "mpylir", "3 = 1 * 2" }, //
		{4, "ext", "4 = 2 ext 1 .. 3" },  //
		{4, "extu", "4 = 2 ext 1 .. 3" },  //
		{0, "reti", "ret" },         // reti -> ret
		{2, "lddw", "2 = (word)1" }, // lddw
		{2, "ldhu", "2 = (half)1" }, // ldhu
		{2, "ldb", "2 = (byte)1" },  // ldb
		{2, "ldbu", "2 = (byte)1" }, // ldbu
		{2, "ldndw", "2 = 1" }, // ldbu
		{2, "ldnw", "2 = 1" }, // ldbu
		{2, "ldw", "2 = (word)1" },  // ldw
		{2, "ldh", "2 = (half)1" },  // ldw
		{2, "stb", "2 = (byte)1" },  // stb
		{2, "stw", "2 = (word)1" },  // stw
		{2, "sth", "2 = (half)1" },  // stw
		{2, "stnw", "2 = (word)1" },  // stw
		{2, "stdw", "2 = (half)1" }, // stw
		{2, "stndw", "2 = (half)1" }, // stw
		{3, "or", "3 = 2 | 1" },
		{3, "shl", "3 = (2 & Oxffffff) << 1" },
		{3, "shr", "3 = (2 & Oxffffff) << 1" },
		{3, "shlmb", "3 = << 1" },
		{4, "set", "4 = 2 .bitset 1 .. 2" }, // set a29,0x1a, 1, a19
		{4, "clr", "4 = 2 .bitclear 1 .. 2" }, // clr a29,0x1a, 1, a19
		{0, "invalid", "" },
		{0, "nop", "" },
		{0, NULL}
	};
	if (!newstr) {
		return false;
	}

	for (i = 0; ops[i].op; i++) {
		if (ops[i].narg) {
			if (argc - 1 != ops[i].narg) {
				continue;
			}
		}
		if (!strcmp (ops[i].op, argv[0])) {
			for (j = k = 0; ops[i].str[j]; j++, k++) {
				if (isdigit (ops[i].str[j])) {
					int index = ops[i].str[j] - '0';
					if (index >= 0 && index < argc) {
						const char *w = argv[index];
						if (!R_STR_ISEMPTY (w)) {
							r_str_cpy (newstr + k, w);
							k += strlen (w) - 1;
						}
					}
				} else {
					newstr[k] = ops[i].str[j];
				}
			}
			newstr[k] = '\0';
			if (argc == 4 && argv[2][0] == '[') {
				strcat (newstr + k, "+");
				strcat (newstr + k + 3, argv[2]);
			}
			return true;
		}
	}

	/* TODO: this is slow */
	newstr[0] = '\0';
	for (i = 0; i < argc; i++) {
		strcat (newstr, argv[i]);
		strcat (newstr, (i == 0 || i == argc - 1) ? " " : ",");
	}
	return false;
}

static char *parse(RAsmPluginSession *aps, const char *data) {
	if (r_str_startswith (data, "|| ")) {
		data += 3;
	}
	if (R_STR_ISEMPTY (data)) {
		return NULL;
	}

	char *buf = strdup (data);

	RListIter *iter;
	char *sp = strchr (buf, ' ');
	size_t nw = 1;
	const char *wa[5] = {0};
	wa[0] = buf;
	RList *list = NULL;
	if (sp) {
		*sp++ = 0;
		list = r_str_split_list (sp, ",", 0);
		char *w;
		r_list_foreach (list, iter, w) {
			wa[nw] = w;
			nw++;
			if (nw == 5) {
				break;
			}
		}
	}
	char *str = malloc (strlen (data) + 128);
	strcpy (str, data);
	replace (nw, wa, str);

	free (buf);
	r_list_free (list);

	return str;
}

RAsmPlugin r_asm_plugin_tms320 = {
	.meta = {
		.name = "tms320",
		.desc = "tms320 pseudo syntax",
		.author = "pancake",
		.license = "LGPL-3.0-only",
	},
	.parse = parse,
};

#ifndef R2_PLUGIN_INCORE
R_API RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_ASM,
	.data = &r_asm_plugin_tms320,
	.version = R2_VERSION};
#endif
