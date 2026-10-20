CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS) -std=c++17
OBJECTS=clock_utils.o timezones.o color_temp.o pi-clock.o FileReader.o yaml_config.o
BINARIES=pi-clock

# Hardware-independent unit tests (no LED matrix library needed)
TEST_OBJECTS=clock_utils.o timezones.o FileReader.o test_pi_clock.o
TEST_BINARY=test_pi_clock

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
RGB_LIB_DISTRIBUTION=/home/pi/git/rpi-rgb-led-matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lyaml-cpp

all : $(BINARIES)

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

pi-clock : $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

test : $(TEST_BINARY)
	./$(TEST_BINARY)

$(TEST_BINARY) : $(TEST_OBJECTS)
	$(CXX) $(TEST_OBJECTS) -o $@

# All the binaries that have the same name as the object file.q
% : %.o $(RGB_LIBRARY)
	$(CXX) $< -o $@ $(LDFLAGS)


%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c -o $@ $<

%.o : %.c
	$(CC) -I$(RGB_INCDIR) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES) $(TEST_OBJECTS) $(TEST_BINARY)

FORCE:
.PHONY: FORCE test
