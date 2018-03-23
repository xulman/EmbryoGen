# sources that make up the program
SOURCES=src/graphics.cpp \
		  src/simulation.cpp \
		  src/agents.cpp \
		  src/rnd_generators.cpp \
		  src/main.cpp

# their objects
OBJECTS=$(SOURCES:.cpp=.o)

# some more hints for the compiler
#DEBUG
#CXXFLAGS=-Wall -Wextra -O0 -pedantic -fopenmp -g

#RELEASE
CXXFLAGS=-Wall -Wextra -O2 -pedantic -fopenmp

# rule to get object files from their sources
.cpp.o:
	g++ $(CXXFLAGS) -c $< -o $@

# default building target ...
all: particles

# now, here's how to build it together
particles: $(OBJECTS)
	g++ $(CXXFLAGS) -o particles $(OBJECTS) -lglut -lGL -lgsl -lgslcblas

clean:
	rm -vf particles $(OBJECTS)
