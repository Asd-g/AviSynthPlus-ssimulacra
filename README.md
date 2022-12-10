## Description

SSIMULACRA - Structural SIMilarity Unveiling Local And Compression Related Artifacts.

Cloudinary's variant of DSSIM, based on Philipp Klaus Krause's adaptation of Rabah Mehdi's SSIM implementation, using ideas from Kornel Lesinski's DSSIM implementation as well as several new ideas.

This is a perceptual metric designed specifically for scoring image compression related artifacts in a way that correlates well with human opinions. For more information, see http://cloudinary.com/blog/detecting_the_psychovisual_impact_of_compression_related_artifacts_using_ssimulacra


SSIMULACRA2 - Major new version of the SSIMULACRA metric.

Changes compared to the original version:
- works in XYB color space
- uses 1-norm and 4-norm (instead of 1-norm and max-norm-after-downscaling)
- penalizes both smoothing and ringing artifacts (instead of only penalizing ringing but not smoothing)
- removed specific blockiness detection

For more information, see https://github.com/cloudinary/ssimulacra2


[libjxl's implementation of SSIMULACRA and SSIMULACRA2](https://github.com/libjxl/libjxl) is used.

### Requirements:

- AviSynth+ r3682 (can be downloaded from [here](https://gitlab.com/uvz/AviSynthPlus-Builds) until official release is uploaded) (r3689 recommended) or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases))

### Usage:

```
ssimulacra(clip reference, clip distorted, bool "simple", int "feature")
```

### Parameters:

- reference, distorted\
    Clips that are used for estimating the psychovisual similarity.\
    They must be in RGB 8/16/32-bit planar format and must have same dimensions.\

- simple (only SSIMULACRA)\
    Whether to skip the edge/grid penalties.\
    True: it's basically just Lab* MS-SSIM with pooling being a weighted sum of the mean error and the max error.\
    Default: False.

- feature\
    The psychovisual similarity of the clips will be stored as frame property.\
    0: SSIMULACRA\
    1: SSIMULACRA2\
    2: Both `0` and `1`.\
    Default: 0.

### Building:

```
Requirements:
    - Git
    - C++17 compiler
    - CMake >= 3.16
```
```
git clone --recurse-submodules https://github.com/Asd-g/AviSynthPlus-ssimulacra

cd AviSynthPlus-ssimulacra\libjxl

cmake -B build -DCMAKE_INSTALL_PREFIX=jxl_install -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DJPEGXL_ENABLE_FUZZERS=OFF -DJPEGXL_ENABLE_TOOLS=OFF -DJPEGXL_ENABLE_MANPAGES=OFF -DJPEGXL_ENABLE_BENCHMARK=OFF -DJPEGXL_ENABLE_EXAMPLES=OFF -DJPEGXL_ENABLE_JNI=OFF -DJPEGXL_ENABLE_OPENEXR=OFF -DJPEGXL_ENABLE_TCMALLOC=OFF -DBUILD_SHARED_LIBS=OFF -DJPEGXL_ENABLE_SJPEG=OFF -G Ninja

cmake --build build

cmake --install build

cd ..

cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja

cmake --build build
```
