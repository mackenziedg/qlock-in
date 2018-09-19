task: clock.c
	gcc clock.c -o ./clock --std=c99 -lsqlite3

clean:
	rm clock *.db

