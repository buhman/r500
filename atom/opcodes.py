_opcodes = """
0	0	null	0
1	dest_src	move	reg
2	dest_src	move	ps
3	dest_src	move	ws
4	dest_src	move	fb
5	dest_src	move	pll
6	dest_src	move	mc
7	dest_src	and	reg
8	dest_src	and	ps
9	dest_src	and	ws
10	dest_src	and	fb
11	dest_src	and	pll
12	dest_src	and	mc
13	dest_src	or	reg
14	dest_src	or	ps
15	dest_src	or	ws
16	dest_src	or	fb
17	dest_src	or	pll
18	dest_src	or	mc
19	shift	shift_left	reg
20	shift	shift_left	ps
21	shift	shift_left	ws
22	shift	shift_left	fb
23	shift	shift_left	pll
24	shift	shift_left	mc
25	shift	shift_right	reg
26	shift	shift_right	ps
27	shift	shift_right	ws
28	shift	shift_right	fb
29	shift	shift_right	pll
30	shift	shift_right	mc
31	dest_src	mul	reg
32	dest_src	mul	ps
33	dest_src	mul	ws
34	dest_src	mul	fb
35	dest_src	mul	pll
36	dest_src	mul	mc
37	dest_src	div	reg
38	dest_src	div	ps
39	dest_src	div	ws
40	dest_src	div	fb
41	dest_src	div	pll
42	dest_src	div	mc
43	dest_src	add	reg
44	dest_src	add	ps
45	dest_src	add	ws
46	dest_src	add	fb
47	dest_src	add	pll
48	dest_src	add	mc
49	dest_src	sub	reg
50	dest_src	sub	ps
51	dest_src	sub	ws
52	dest_src	sub	fb
53	dest_src	sub	pll
54	dest_src	sub	mc
55	setport	setport	ati
56	setport	setport	pci
57	setport	setport	sysio
58	setregblock	setregblock	0
59	src	setfbbase	0
60	dest_src	compare	reg
61	dest_src	compare	ps
62	dest_src	compare	ws
63	dest_src	compare	fb
64	dest_src	compare	pll
65	dest_src	compare	mc
66	switch	switch	0
67	1x16	jump	always
68	1x16	jump	equal
69	1x16	jump	below
70	1x16	jump	above
71	1x16	jump	beloworequal
72	1x16	jump	aboveorequal
73	1x16	jump	notequal
74	dest_src	test	reg
75	dest_src	test	ps
76	dest_src	test	ws
77	dest_src	test	fb
78	dest_src	test	pll
79	dest_src	test	mc
80	1x8	delay	millisec
81	1x8	delay	microsec
82	1x8	calltable	0
83	1x8	repeat	0
84	dest	clear	reg
85	dest	clear	ps
86	dest	clear	ws
87	dest	clear	fb
88	dest	clear	pll
89	dest	clear	mc
90	0	nop	0
91	0	eot	0
92	mask	mask	reg
93	mask	mask	ps
94	mask	mask	ws
95	mask	mask	fb
96	mask	mask	pll
97	mask	mask	mc
98	1x8	postcard	0
99	1x8	beep	0
100	0	savereg	0
101	0	restorereg	0
102	set_data_block	setdatablock	0
103	dest_src	xor	reg
104	dest_src	xor	ps
105	dest_src	xor	ws
106	dest_src	xor	fb
107	dest_src	xor	pll
108	dest_src	xor	mc
109	dest_src	shl	reg
110	dest_src	shl	ps
111	dest_src	shl	ws
112	dest_src	shl	fb
113	dest_src	shl	pll
114	dest_src	shl	mc
115	dest_src	shr	reg
116	dest_src	shr	ps
117	dest_src	shr	ws
118	dest_src	shr	fb
119	dest_src	shr	pll
120	dest_src	shr	mc
121	debug	debug	0
"""

def str_to_i(s):
    d = {
        "reg": 0,
        "ps": 1,
        "ws": 2,
        "fb": 3,
        "id": 4,
        "imm": 5,
        "pll": 6,
        "mc": 7,
    }
    if s in d:
        return d[s]
    else:
        return s

opcodes = {
    int(k): (a, b, str_to_i(c))
    for k, a, b, c in map(str.split, _opcodes.strip().split("\n"))
}
