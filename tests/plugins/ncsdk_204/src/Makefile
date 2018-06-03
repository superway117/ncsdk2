CXX = g++
CC = gcc


CFLAGS := -O2 -Wall -g -DDEBUG_ION
INSTALLDIR := ${DESTDIR}/usr/local


INC = -I../include


LDFLAGS=-lpthread -lrt -ldl -lstdc++

LD_LIBRARY_PATH=/usr/local/lib



SRCS = ../utils/utils.c \
			../utils/utils_ion.c 
#TMP = ../utils/tmp.c

ifeq ($(accept),1)
  all: movi_async movi_sync  movi_slow movi_multigraph
else
  all: icv_sync_ion icv_async_ion icv_graph_ion icv_async_noion
  CFLAGS+=-DTEST_USE_ION
endif

.c.o:
	$(CC) $(INC) -c $(CFLAGS) $< $(SRCS) $(TMP)

icv_sync_ion: icv_sync_ion.o utils.o utils_ion.o test_runner.o
	$(CC) $(LDFLAGS) -o $@ icv_sync_ion.o utils.o  utils_ion.o  test_runner.o -lion -lmvnc -lbsl -pthread -ljson-c -lm

icv_async_ion: icv_async_ion.o utils.o utils_ion.o test_runner.o
	$(CC) $(LDFLAGS) -o $@ icv_async_ion.o utils.o  utils_ion.o  test_runner.o -lion -lmvnc -lbsl -pthread -ljson-c -lm

icv_graph_ion: icv_graph_ion.o utils.o utils_ion.o test_runner.o
	$(CC) $(LDFLAGS) -o $@ icv_graph_ion.o utils.o  utils_ion.o  test_runner.o -lion -lmvnc -lbsl -pthread -ljson-c -lm

icv_async_noion: icv_async_noion.o utils.o utils_ion.o test_runner.o
	$(CC) $(LDFLAGS) -o $@ icv_async_noion.o utils.o  utils_ion.o  test_runner.o -lion -lmvnc -lbsl -pthread -ljson-c -lm


movi_sync: movi_sync.o utils.o test_runner.o
	$(CC) $(LDFLAGS) -o $@ movi_sync.o utils.o test_runner.o -lmvnc -pthread -ljson-c -lm

movi_async: movi_async.o utils.o test_runner.o
	$(CC) $(LDFLAGS) -o $@ movi_async.o utils.o  test_runner.o -lmvnc -pthread -ljson-c -lm

movi_slow: movi_slow.o utils.o test_runner.o
	$(CC) $(LDFLAGS) -o $@ movi_slow.o utils.o test_runner.o -lmvnc -pthread -ljson-c -lm

movi_multigraph: movi_multigraph.o utils.o test_runner.o
	$(CC) $(LDFLAGS) -o $@ movi_multigraph.o utils.o test_runner.o -lmvnc -pthread -ljson-c -lm

clean:
	rm -f *.o va result*.bin rgb*.bin movi_multigraph movi_slow \
		test_graph movi_sync movi_async icv_sync_ion icv_async_ion icv_graph_ion icv_async_noion

