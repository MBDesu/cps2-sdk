.PHONY: all tools sdk clean

all: tools sdk

tools:
	cmake -S tools -B tools/build
	cmake --build tools/build -j

sdk: tools
	cmake -S sdk -B sdk/build
	cmake --build sdk/build -j

clean:
	rm -rf tools/build sdk/build
