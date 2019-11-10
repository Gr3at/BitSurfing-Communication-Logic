 setup config

----Testing Raspberry Pi-BeagleBoneBlack (RPi-BBB) Communication----
MAXRECVSTRING 1200
SUCCESSES_TO_FINISH 10
READING_STEP  **different for each directory (e.g. 100,10,1000)**
PACKET_HEADERS_SIZE 200

###############################
READING_STEP | Bytes_Considered
------------- -----------------
1000			1	ok	(3/3 B-R, 9/10 (lost 1st) R-B)
500				2	ok	(7/10 (lost 1st) B-R, 8/8 R-B)
333				3
250				4	ok	(3/10 (lost 1st) B-R, 4/7 R-B)
200				5
166				6	ok	(2/20 (lost 1st) B-R, 10/15 (5 losses are previous msgs) R-B)	('make test' BBB first)
142				7
125				8	ok	(0/10 B-R, 2/4 (lost 1st) R-B)
111				9
100				10	ok	(2/10 B-R, 0/6 (lost 1st) R-B)
90				11
83				12
76				13
71				14
66				15	ok	(1/20 (lost 1st) B-R, 2/17 (9 losses are previous msgs) R-B)
62				16
58				17
55				18
52				19
50				20	ok	(0/14 B-R, 5/20 (lost 1st) R-B)	('make test' RPi first)
47				21
45				22
43				23
41				24
40				25	ok	(0/20 B-R, 5/19 (5 losses are previous msgs) R-B) ('make test' BBB first)
38				26
37				27
35				28
34				29
33				30	ok	(1/11 B-R, 2/20 (lost 1st) R-B)	('make test' BBB first)
20				50
10				100
5				200
2				500
1				1000

B-R = BBB-->RPi

---- file exchange ----
##### recRPi.txt #####

0 | 0 | 57347 | bitsPassedTilMsg: 20
58008 | 60506 | 57481 | bitsPassedTilMsg: 4
59713 | 58472 | 57395 | bitsPassedTilMsg: 7
58038 | 59666 | 57354 | bitsPassedTilMsg: 8
59433 | 60927 | 61439 | bitsPassedTilMsg: 9

resutls after commenting out gpioStuff.c gpio_free(...) function
commented back in, check if different results
