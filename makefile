all:
	gcc -pthread -o server/server_PFS server/server_PFS.c
	gcc -pthread -o client/client_PFS client/client_PFS.c
	cp client/client_PFS A/client_PFS
	cp client/client_PFS B/client_PFS
	cp client/client_PFS C/client_PFS
	cp client/client_PFS D/client_PFS
	cp client/client_PFS E/client_PFS
	cp testFiles/mountain_1.jpg A
	cp testFiles/mountain_4.jpg A
	cp testFiles/text1.txt A
	cp testFiles/mountain_2.jpg B
	cp testFiles/text2.txt B
	cp testFiles/mountain_3.jpg C
	cp testFiles/text3.txt C
	cp testFiles/mountain_4.jpg D
	cp testFiles/text4.txt D
	cp testFiles/mountain_5.jpg E
	cp testFiles/mountain_6.jpg E
	cp testFiles/text5.txt E

clean:
	rm -rf client/client_PFS \
	server/server_PFS \
	A/* \
	B/* \
	C/* \
	D/* \
	E/*
