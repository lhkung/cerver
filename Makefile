mkdir = mkdir
bindir = ./bin
rm = rm -r
LIBRARY = $(bindir)/server.o $(bindir)/tcpconnection.o $(bindir)/threadpool.o
TARGETS = $(LIBRARY) $(bindir)/httpserver
all: $(bindir) $(TARGETS)
clean:
	rm -r bin

$(bindir):
	$(mkdir) $(bindir);
$(bindir)/server.o: src/server.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/tcpconnection.o: src/tcpconnection.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/threadpool.o: src/threadpool.cpp
	g++ -Wall -std=c++17 -lpthread -c $^ -o $@
$(bindir)/httpserver: src/httpserver.cpp $(LIBRARY)
	g++ -Wall -std=c++17 -lpthread $^ -o $@