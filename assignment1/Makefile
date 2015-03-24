storage_mgr.exe: storage_mgr.o test_assign1_1.o dberror.o
	gcc -o run_test storage_mgr.o test_assign1_1.o dberror.o

dberror.o: dberror.c dberror.h
	gcc -c dberror.c

storage_mgr.o: storage_mgr.c storage_mgr.h
	gcc -c storage_mgr.c

test_assign1_1.o: test_assign1_1.c storage_mgr.h
	gcc -c test_assign1_1.c

clean:
	rm run_test storage_mgr.o test_assign1_1.o dberror.o



