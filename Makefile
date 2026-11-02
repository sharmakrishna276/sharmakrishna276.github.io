BUILD := build
SITE  := _site
SSG   := $(BUILD)/ssg
SRCS  := CMakeLists.txt $(wildcard src/*.cpp src/*.hpp)

.PHONY: all site serve asan clean

all: site

$(SSG): $(SRCS)
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD)

site: $(SSG)
	rm -rf $(SITE)
	$(SSG) . $(SITE)

serve: site
	python3 -m http.server 8000 --directory $(SITE)

# Build and run under AddressSanitizer + UBSan.
asan:
	cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
	  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
	cmake --build build-asan
	rm -rf $(SITE)
	build-asan/ssg . $(SITE)

clean:
	rm -rf $(BUILD) build-asan $(SITE)
