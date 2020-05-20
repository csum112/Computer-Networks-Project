all:
	$(MAKE) clean
	$(MAKE) hasher
	$(MAKE) compile
	$(MAKE) build
hasher:
	cd shared/filesystem/hashlib2plus/trunk/src/ && $(MAKE)
	cp shared/filesystem/hashlib2plus/trunk/src/libhl++.a bin/libhl++.bin
	cd shared/filesystem/hashlib2plus/trunk/src/ && $(MAKE) clean

compile:
	gcc -c shared/client-database/sqlite3/sqlite3.c -lpthread -ldl -I ./shared -o bin/sqlite3.bin
	g++ -std=c++17 -c shared/client-database/database.cpp -I ./shared -o bin/database.bin
	g++ -std=c++17 -c shared/filesystem/filesystem.cpp -I ./shared -o bin/filesystem.bin
	g++ -std=c++17 -c shared/Patcher/Patcher.cpp -I ./shared -o bin/Patcher.bin
	g++ -std=c++17 -c shared/bundler/bundler.cpp -I ./shared/bundler -o bin/bundler.bin
	clear
build:
	g++ -std=c++17 client/cli.cpp bin/*.bin -o mysvn
	g++ -std=c++17 server/server.cpp bin/*.bin -lpthread -o mysvnserver
clean:
	rm bin/*.bin || echo -n ''
install:
	cp mysvn /usr/local/bin/mysvn
	cp mysvnserver /usr/local/bin/mysvnserver

uninstall:
	rm /usr/local/bin/mysvn || echo -n ''
