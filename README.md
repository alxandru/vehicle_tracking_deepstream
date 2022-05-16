# Vehicle Detection and Tracking with Yolov4 and DeepStream SDK

This application detects and tracks vehicles in a roundabout from a fixed camera stream using [Nvidia DeepStream SDK](https://developer.nvidia.com/deepstream-sdk) and sends information about the entries and the exits along with the vehicle ids to a Kafka Message Bus in order for a client application to process the data.

For detection the application uses a [custom trained Yolov4-Tiny network](https://github.com/alxandru/yolov4_roundabout_traffic) based on [RoundaboutTraffic](https://github.com/alxandru/yolov4_roundabout_traffic/tree/main/data) dataset. [DeepStream-Yolo](https://github.com/marcoslucianops/DeepStream-Yolo) was used to improve inference performance.

For tracking the [NvDCF](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvtracker.html#nvdcf-tracker) tracker as well as the [DeepSORT](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_gst-nvtracker.html#deepsort-tracker-alpha) tracker were tested and used in the application.

A Kafka producer based on [librdkafka](https://github.com/edenhill/librdkafka) library was implemented to send information about the tracked vehicles to the message bus.


## Table of contents
---
* [Requirements](#requirements)
* [Installation](#install)
* [Configuration](#config)
* [Usage](#usage)


<a name="requirements"></a>

## Requirements
---
* [Nvidia Jetson Nano](https://developer.nvidia.com/embedded/learn/get-started-jetson-nano-devkit) (not mandatory)
* [JetPack 4.6](https://developer.nvidia.com/embedded/jetpack)
* [NVIDIA DeepStream SDK 6.0](https://developer.nvidia.com/deepstream-sdk)


<a name="install"></a>

## Installation
---

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
---

The application uses a yolov4-tiny custom model for vehicle detection and mars-small128 model for DeepSORT tracking algorithm.
Both models will be downloaded by running the following script:

```bash
$ bash models/get_models.sh
```

<a name="usage"></a>

## Usage
---

For testing purposes we download and use this [video](https://drive.google.com/file/d/1GnGOLN_1nlq1-yttD_uk_zJzgfr6vt8Q/view?usp=sharing).

