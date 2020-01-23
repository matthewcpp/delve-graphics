# delve-graphics

Graphics demos and shader programming

### Building
You will need to download the [Vulkan SDK](https://vulkan.lunarg.com/).  
You will need to have Conan and Cmake installed.

```shell script
# Create and enter build directory
mkdir build && cd build

# Install package dependencies from conan
conan install ..

# Generate project files, ensuring that the path to the Vulkan SDK is specified
cmake .. -DVULKAN_SDK=/path/to/vulkan/sdk
```
### Ubuntu
Install the following development packages

```shell script
sudo apt-get install libxcb1-dev xorg-dev
```

#### MacOS
When running the application, ensure that the following environment variables are set:

| Name | Value |
| ----------- | ----------- |
| `DYLD_LIBRARY_PATH` | `<VULKAN_SDK>/macOS/lib`
| `VK_ICD_FILENAMES` | `<VULKAN_SDK>/macOS/etc/vulkan/icd.d/MoltenVK_icd.json` |
| `VK_LAYER_PATH` | `<VULKAN_SDK>/macOS/etc/vulkan/explicit_layer.d` |

