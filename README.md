# Lab

This is a place where I've been experimenting with image processing and audio processing.

## Image Lab

In this part, I've been basically trying to implement different image processing algorithms, mostly on the gpu as compute shaders, and sometimes on cpu as the algorithm doesn't really fit the massively parallel nature of gpu computing.

It's basically set up as a processing stack. We add processes on top of another.

So to apply processing to an image, we first have to add a "Add Image" process for example, and then all the subsequent added process will apply to the previous one.

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

