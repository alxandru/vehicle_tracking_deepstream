# Vehicle Detection and Tracking with Yolov4 and DeepStream SDK

![Alt Text](https://media.giphy.com/media/B45c0SelzSWaY6OgnY/giphy.gif)

This application detects and tracks vehicles in a roundabout from a fixed camera stream using [Nvidia DeepStream SDK](https://developer.nvidia.com/deepstream-sdk) and sends information about the entries and the exits along with the vehicle ids to a Kafka Message Bus in order for a client application to process the data.

For detection the application uses a [custom trained Yolov4-Tiny network](https://github.com/alxandru/yolov4_roundabout_traffic) based on [RoundaboutTraffic](https://github.com/alxandru/yolov4_roundabout_traffic/tree/main/data) dataset. [DeepStream-Yolo](https://github.com/marcoslucianops/DeepStream-Yolo) was used to improve inference performance.

For tracking the [Discriminative Correlation Filter](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvtracker.html#nvdcf-tracker) tracker as well as the [DeepSORT](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvtracker.html#deepsort-tracker-alpha) tracker were tested and used.

A Kafka producer based on [librdkafka](https://github.com/edenhill/librdkafka) library was implemented to send information about the tracked vehicles to the message bus.


## Table of contents

* [Requirements](#requirements)
* [Installation](#install)
* [Configuration](#config)
* [Usage](#usage)
* [Discussion](#discussion)


<a name="requirements"></a>

## Requirements

* [Nvidia Jetson Nano](https://developer.nvidia.com/embedded/learn/get-started-jetson-nano-devkit) (not mandatory)
* [JetPack 4.6](https://developer.nvidia.com/embedded/jetpack)
* [NVIDIA DeepStream SDK 6.0](https://developer.nvidia.com/deepstream-sdk)


<a name="install"></a>

## Installation


Once we have all the requirements installed we download the repo and the third parties:

```bash
$ git clone https://github.com/alxandru/vehicle_tracking_deepstream.git
$ cd vehicle_tracking_deepstream
$ git submodule update --init --recursive
```

[DeepStream-Yolo](https://github.com/marcoslucianops/DeepStream-Yolo) and [librdkafka](https://github.com/edenhill/librdkafka) are used by the application as described in the introduction. First we need to configure and compile them.


Configure librdkafka:

```bash
$ cd 3pp/librdkafka
$ ./configure
$ cd ../../
```

Compile both third parties:

```bash
$ CUDA_VER=10.2 make subsystem
```

librdkafka needs installation:

```bash
$ sudo CUDA_VER=10.2 make install
```

Finally we compile the application:

```bash
$ CUDA_VER=10.2 make
```

<a name="config"></a>

## Configuration


The application uses a yolov4-tiny custom model for vehicle detection and mars-small128 model for DeepSORT tracking algorithm.
Both models will be downloaded by running the following script:

```bash
$ bash models/get_models.sh
```

<a name="usage"></a>

## Usage

The application takes as input a video in h264 format and outputs a mp4 video with the annotations:

```bash
$ ./bin/vehicle-tracking-deepstream test002.h264 output.mp4
```

For testing purposes you can download this [video](https://drive.google.com/file/d/1GnGOLN_1nlq1-yttD_uk_zJzgfr6vt8Q/view?usp=sharing) and use it as input for the app.

For tracking, the DeepStream discriminative correlation filter (DCF) is used but it can be changed to DeepSORT tracker by modifying the `cfg/tracker_config.txt` file. Just uncomment the `ll-config-file` line for DeepSORT and comment it for NvDCF tracker:

```bash
#ll-config-file=config_tracker_NvDCF_perf.yml
ll-config-file=config_tracker_DeepSORT.yml
```

Here is a video snippet with the NvDCF tracker (click on image to open the Youtube video):

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/7yzgS53jE74/hqdefault.jpg)](https://www.youtube.com/watch?v=7yzgS53jE74)

And the same video but with DeepSORT tracker activated:

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/dRvLdxYPX1k/hqdefault.jpg)](https://www.youtube.com/watch?v=dRvLdxYPX1k)

<a name="discussion"></a>

## Discussion


The main issues for the trackers in this fixed camera scenario are the occlusions and the changes in angle of the vehicles.
The NvDCF tracker suffers more from the occlusion problem. On the other hand the DeepSORT tracker handles the changes in vehicle angles worse.
Nevertheless, in general, the two tracking algorithms perform similarly. At the end of the day almost the same number of vehicles are lost due to re-identification.

The DeepSORT algorithm uses mars-small128 model which originally was trained for recognizing humans. As a next step, it would be interesting to train a model for recognizing cars for DeepSORT tracker and check how much it would improve.

In terms of number of frames processed per second (FPS) there is no big difference between the two trackers. With DeepSORT the application processes around 14 FPS on average, whereas with NvDCF processes around 13 FPS.