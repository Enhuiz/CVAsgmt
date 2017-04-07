# Computer Vision Assignment

This is a web image processing system based on boost asio, which is used to present the assignment of computer vision course of Software Engineering Department, SCUT, given by Dr. Shaowu Peng.

# Build


## Dependencies

- [OpenCV 3.2](http://opencv.org/)
- [OpenCV Contrib](https://github.com/opencv/opencv_contrib)
- [Boost Asio](http://www.boost.org/)


## CMake

**under ./**

```
$ mkdir build
$ cd build
$ cmake ..
```

## Make (Linux)

**under ./build**

```
$ make
```

## Run

**under ./**

```
$ bin/app
```

## Don't Do This
```
# It's necessary to call app under ./, for app refers to some relative paths.
# The following cmds are not right, which will cause a 404 problem
$ cd bin
$ app
```
