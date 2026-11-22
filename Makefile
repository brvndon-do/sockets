CXX := clang++
CXXFLAGS := -std=c++23 -Wall -Wextra -g
TARGETS := server client showip

all: $(TARGETS)

%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean