task: clock.c task_utils.c task_utils.h
	gcc clock.c task_utils.c -o ./clock --std=c99 -lsqlite3

clean:
	rm clock *.db

