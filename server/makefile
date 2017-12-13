LIBS = -lmysqlclient -lpthread
OBJS = server.o mystdlib.o packetProcessing.o mysql_sum.o
ALL: ${OBJS}
	gcc -o OUT ${OBJS} ${LIBS}
clean:
	rm -f OUT ${OBJS}

