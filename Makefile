CXXFLAGS += -I$(VULKAN_SDK)/include
LDFLAGS += -L$(VULKAN_SDK)/lib -lvulkan

libNullVRS_layer.so: NullVRS_layer.cpp
	c++ $(CXXFLAGS) -shared -fPIC -std=c++11 NullVRS_layer.cpp -o libNullVRS_layer.so $(LDFLAGS)
