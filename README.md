Vulkan Layer: NullVRS
=====================
A simple Vulkan layer which intercepts Variable Rate Shading commands, replacing them with no-ops.

This layer was made primarily to get Doom: The Dark Ages to run on an AsRock BC-250, which is a
single-board computer with a cut down PS5 APU. This hardware does not support Vulkan's Variable Rate
Shading extension, but does support everything else the game needs such as RT. 

Doom: The Dark Ages requires Variable Rate Shading as of Update 2, and crashes if it is not
available (until Update 4 fixed this).

This layer may help other systems which would encounter the same issue, such as Vega or RDNA1 GPUs
using Mesa's emulate_rt, or a hacked PS5.

I do not know of any other games this layer would be useful for.

**NOTE: NullVRS is no longer needed for Doom: The Dark Ages as of Update 4!**
I will keep this repo up anyways just in case it is useful for something else.


Installation & Usage Instructions
=========================
1. To install the layer:
   - Take the .json and .so files from the release or from your build.
   - Place them in a Vulkan Implicit Layer load directory, such as 
     `"~/.local/share/vulkan/implicit_layer.d/"`
   - This installs the layer, but no game will load it unless instructed via the next step.
2. To enable the layer:
   - Before running the game, set the following environment variable: `ENABLE_VK_NULLVRS_1=1`
   - In Steam, this can be done by right clicking the game, Properties, and placing the following
     in the launch options: `ENABLE_VK_NULLVRS_1=1 %command%`
   
I do not recommend setting the ENABLE_VK_NULLVRS_1 envrionemnt variable in any global, system-wide,
or user-wide environment. I do not know how various anticheats would react to the layer being
loaded. So, please only use it on a game-by-game basis.

Build Instructions
==================
Currently I have only built this for Linux.

1. Get the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) and set it up in a folder.
2. Set the environment variable `VULKAN_SDK` to the path of the extracted Vulkan SDK.
3. Run `make`
4. An .so is produced. This is the layer.

Technical info on Doom: The Dark Ages issue
===========================================
Doom: The Dark Ages tries to get the address of vkCmdSetFragmentShadingRateKHR(), which is
unavailable due to not being supported on BC-250. This check is not gated behind checking the
Vulkan extensions to see if Variable Rate Shading is supported, causing a crash. This layer
works around this by causing vkCmdSetFragmentShadingRateKHR() to resolve to a function that does
nothing. Outside of the forced checking at launch, the use of Variable Rate Shading in Doom: TDA
is completely optional, therefore there is no side effect on the game itself running without it.

**NOTE: This issue was fixed in Doom TDA Update 4!**


Other Notes
===========
This layer was based on RenderDoc's "Brief guide to Vulkan layers" 
[tutorial](https://renderdoc.org/vulkan-layer-guide.html), but updated to use the Vulkan dispatch
table helper structs now part of more recent Vulkan SDK. You could use this project as a pretty
simple "Hello World" Vulkan Layer sample for 2026-era Vulkan SDK.
