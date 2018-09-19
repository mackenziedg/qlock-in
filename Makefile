task: clock.c
	gcc clock.c -o ./clock --std=c99 -lsqlite3 -O3 -Os

clean:
	rm clock *.db

