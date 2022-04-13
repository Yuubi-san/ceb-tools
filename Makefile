override CPPFLAGS := -DNDEBUG -O3 -fPIE -Wall -Wextra -Wpedantic -Wconversion \
  -Wcast-align -Wformat=2 -Wstrict-overflow=5 -Wsign-promo $(CPPFLAGS)
override CXXFLAGS := --std=c++2a -Woverloaded-virtual $(CXXFLAGS)
override LDLIBS   := -lcrypto $(LDLIBS)

ceb2sqlgz: ceb2sqlgz.o unaesgcm/unaesgcm.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
	strip --strip-all $@
	if which termux-elf-cleaner > /dev/null; then termux-elf-cleaner $@; fi

ceb2sqlgz.cpp: unaesgcm/unaesgcm.hpp unaesgcm/hex.hpp

.PHONY: clear
clear:
	rm -f ceb2sqlgz ceb2sqlgz.o unaesgcm/unaesgcm.o
