setup config

----Testing 2 Raspberry Pis (RPis) Communication----
MAXRECVSTRING 1200
SUCCESSES_TO_FINISH 10
READING_STEP  **different for each directory (e.g. 100,10,1000)**
PACKET_HEADERS_SIZE 200

###############################
READING_STEP | Bytes_Considered
------------- -----------------
1000			1	ok (2/2 N-O, 20/20 O-N)
500				2	ok (5/5 N-O, 7/10 O-N)
333				3
250				4	ok (14/16 (lost 1st) N-O, 1/20 O-N) !!!!! (REPEAT)
200				5
166				6	ok (8/14 (lost 1st) N-O, 2/20 O-N)
142				7
125				8	ok (7/20 (6 losses are previous) N-O, 5/15 O-N)
111				9
100				10	todo
90				11
83				12
76				13 
71				14
66				15	todo	
62				16
58				17
55				18
52				19
50				20	todo
47				21
45				22
43				23
41				24
40				25	todo
38				26
37				27
35				28
34				29
33				30	ok (2/18 N-O, 2/20 O-N)

N-O = RPi New (UK) --> RPi Old(CH)

!!!!! Also run test with zero sleep and just log next msg !!!!!