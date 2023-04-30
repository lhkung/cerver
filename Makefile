mkdir = mkdir
bindir = ./bin
rm = rm -r
LIBRARY = $(bindir)/server.o $(bindir)/tcpconnection.o $(bindir)/threadpool.o $(bindir)/httprequest.o $(bindir)/httpresponse.o $(bindir)/utils.o $(bindir)/httpserver.o
TARGETS = $(LIBRARY) $(bindir)/angie
all: $(bindir) $(TARGETS)
clean:
	rm -r bin

$(bindir):
	$(mkdir) $(bindir);
$(bindir)/utils.o : src/utils.cpp
	g++ -Wall -std=c++17 -c $^ -o $@ -g
$(bindir)/httprequest.o : src/httprequest.cpp
	g++ -Wall -std=c++17 -c $^ -o $@ -g
$(bindir)/httpresponse.o : src/httpresponse.cpp
	g++ -Wall -std=c++17 -c $^ -o $@ -g
$(bindir)/server.o: src/server.cpp
	g++ -Wall -std=c++17 -c $^ -o $@ -g
$(bindir)/tcpconnection.o: src/tcpconnection.cpp
	g++ -Wall -std=c++17 -c $^ -o $@ -g
$(bindir)/threadpool.o: src/threadpool.cpp
	g++ -Wall -std=c++17 -c $^ -o $@ -g
$(bindir)/httpserver.o: src/httpserver.cpp
	g++ -Wall -std=c++17 -c $^ -o $@ -g
$(bindir)/angie: src/angie.cpp $(LIBRARY)
	g++ -Wall -std=c++17 -lpthread $^ -o $@ -g