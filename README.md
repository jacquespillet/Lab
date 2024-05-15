# Lab

This is a place where I've been experimenting with image processing and audio processing.

## Build
### Requirements : 
    Visual Studio (Tested only on Visual Studio 2019)

### Commands : 
```
### Clone the repo and checkout to the latest branch
git clone --recursive https://github.com/jacquespillet/Lab.git
cd Lab

### Generate the solution
mkdir build
cd build
cmake ../

### Build
cd ..
BuildDebug.bat / BuildRelease.bat

First build may take a while because it's going to build all the dependencies with the project.

```


## Image Lab

![Banner](https://github.com/jacquespillet/Lab/blob/main/resources/Gallery/ImageLab/ImageLabBanner.png?raw=true)

In this part, I've been basically trying to implement different image processing algorithms, mostly on the gpu as compute shaders, and sometimes on cpu as the algorithm doesn't really fit the massively parallel nature of gpu computing.

It's basically set up as a processing stack. We add processes on top of another.

So to apply processing to an image, we first have to add a "Add Image" process for example, and then all the subsequent added process will apply to the previous one.

Have a look at the [gallery](https://github.com/jacquespillet/Lab/tree/main/resources/Gallery) to see some of the output images of each process.

Here's a list of all the processes implemented : 

* Color Contrast Stretch

* Gray Scale Contrast Stretch

* Negative

* Local Threshold

* Threshold

* Quantization

* Transform

* Resampling

* Add Noise

* Smoothing Filter

* Sharpen Filter

* Sobel Filter

* Median Filter

* Min/Max Filter

* Gaussian Blur

* Half Toning

* Dithering

* Erosion

* Add Gradient

* Dilation

* Add Image

* Multiply Image

* Gaussian Pyramid

* Laplacian Pyramid

* Pen Draw

* Hard Composite

* Seam Carving Resize

* Patch Inpainting

* Multi-Resolution Composite

* Curve Grading

* Add Color

* Laplacian Of Gaussian

* Difference Of Gaussians

* Canny Edge Detector

* Arbitrary Filter

* Gamma Correction

* Color Distance

* Edge Linking

* Region Grow

* Region Properties

* Error Diffusion Halftoning

* K-Means Cluster

* Super Pixels Cluster

* Hough Transform

* Polygon Fitting

* Otsu Threshold

* Gradient

* Equalize

* FFT Blur


## Audio Lab

The Audio Lab is a smalll synthetizer that allows to author different instruments and to create tracks. 

It's very limited in features, and I would love to improve it when I have some more time.

Here's the interface : 
![Interface](https://github.com/jacquespillet/Lab/blob/main/resources/Gallery/AudioLab/Interface.png?raw=true)

We can have multiple clips, each clip uses an instrument. 

There are multiple settings for defining an instrument : 

Enveloppe : 

This defines how the key behaves throughout its lifetime. How loud it starts, wether it fades in at the start, fades out at the end, and other options...

![Enveloppe](https://github.com/jacquespillet/Lab/blob/main/resources/Gallery/AudioLab/Enveloppe.png?raw=true)

Wave : 

This allows to define the shape of the sound wave. It can be an accumulation of multiple frequency functions (Sine, Cosine, Saw Tooth, Triangle, Square, Noise, Impulse, Sawn)

![Wave](https://github.com/jacquespillet/Lab/blob/main/resources/Gallery/AudioLab/Wave.PNG?raw=true)

There's also a possibility to add a high-pass filter, and to do frequency decay to create drums.

