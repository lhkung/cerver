mkdir = mkdir
bindir = ./bin
rm = rm -r
LIBRARY = $(bindir)/server.o $(bindir)/tcpconnection.o $(bindir)/threadpool.o $(bindir)/httprequest.o $(bindir)/httpresponse.o $(bindir)/utils.o $(bindir)/httpserver.o $(bindir)/memtable.o $(bindir)/commitlog.o $(bindir)/tabula.o
TARGETS = $(LIBRARY) $(bindir)/helloworld $(bindir)/webserver
all: $(bindir) $(TARGETS)
clean:
	rm -r bin

$(bindir):
	$(mkdir) $(bindir);
$(bindir)/utils.o : src/utils.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/httprequest.o : src/httprequest.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/httpresponse.o : src/httpresponse.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/server.o: src/server.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/tcpconnection.o: src/tcpconnection.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/threadpool.o: src/threadpool.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/httpserver.o: src/httpserver.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/memtable.o: src/tabula/memtable.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/commitlog.o: src/tabula/commitlog.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/tabula.o: src/tabula/tabula.cpp
	g++ -Wall -std=c++17 -c $^ -o $@
$(bindir)/helloworld: src/helloworld.cpp $(LIBRARY)
	g++ -Wall -std=c++17 -lpthread $^ -o $@
$(bindir)/webserver: src/webserver.cpp $(LIBRARY)
	g++ -Wall -std=c++17 -lpthread $^ -o $@