CFLAGS = -std=c++17 -O2

LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

Triangle: main.cpp requirement.cpp debugshit.cpp p_device.cpp
	g++ $(CFLAGS) -o Triangle main.cpp requirement.cpp debugshit.cpp p_device.cpp $(LDFLAGS)

.PHONY: test clean

test: Triangle
	./Triangle

clean:
	rm -f Triangle
