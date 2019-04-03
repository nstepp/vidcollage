

CC=g++
CPPFLAGS=-O2
LDFLAGS=-lopencv_core -lopencv_imgproc -lopencv_highgui

vidcollage: vidcollage.o
	$(CC) $(CPPFLAGS) $(LDFLAGS) vidcollage.o -o vidcollage

clean:
	@rm -f *.o core

