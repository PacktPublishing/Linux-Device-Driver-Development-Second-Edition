# Linux Device Driver Development-Second Edition

<a href="https://www.packtpub.com/product/linux-device-driver-development-second-edition/9781803240060?utm_source=github&utm_medium=repository&utm_campaign=9781803240060"><img src="https://static.packt-cdn.com/products/9781803240060/cover/smaller" alt="Understanding and installing toolchains" height="256px" align="right"></a>

This is the code repository for [Linux Device Driver Development-Second Edition](https://www.packtpub.com/product/linux-device-driver-development-second-edition/9781803240060?utm_source=github&utm_medium=repository&utm_campaign=9781803240060), published by Packt.

**Everything you need to start with device driver development for Linux kernel and embedded Linux**

## What is this book about?
Linux is by far the most-used kernel on embedded systems. Thanks to its subsystems, the Linux kernel supports almost all of the application fields in the industrial world. This updated second edition of Linux Device Driver Development is a comprehensive introduction to the Linux kernel world and the different subsystems that it is made of, and will be useful for embedded developers from any discipline. 

This book covers the following exciting features:
* Download, configure, build, and tailor the Linux kernel
* Describe the hardware using a device tree
* Write feature-rich platform drivers and leverage I2C and SPI buses
* Get the most out of the new concurrency managed workqueue infrastructure
* Understand the Linux kernel timekeeping mechanism and use time-related APIs
* Use the regmap framework to factor the code and make it generic
* Offload CPU for memory copies using DMA
* Interact with the real world using GPIO, IIO, and input subsystems

If you feel this book is for you, get your [copy](https://www.amazon.com/dp/1803240067) today!

<a href="https://www.packtpub.com/?utm_source=github&utm_medium=banner&utm_campaign=GitHubBanner"><img src="https://raw.githubusercontent.com/PacktPublishing/GitHub/master/GitHub.png" 
alt="https://www.packtpub.com/" border="5" /></a>

## Instructions and Navigations
All of the code is organized into folders. For example, Chapter02.

The code will look like the following:
```
struct mutex {
	atomic_long_t owner;
	spinlock_t wait_lock;
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
	struct optimistic_spin_queue osq; /* Spinner MCS lock */
```

**Following is what you need for this book:**
This Linux OS book is for embedded system and embedded Linux enthusiasts/developers who want to get started with Linux kernel development and leverage its subsystems. Electronic hackers and hobbyists interested in Linux kernel development as well as anyone looking to interact with the platform using GPIO, IIO, and input subsystems will also find this book useful.

With the following software and hardware list you can run all code files present in the book (Chapter 1-17).
### Software and Hardware List
| Chapter | Software required | OS required |
| -------- | ------------------------------------ | ----------------------------------- |
| 1-17 | A computer with good network bandwidth and enough space and RAM to download and build Linux kernel | Preferably any Debian distribution |
| 1-17 | Any cortex -A embedded board available on market (for example UDOO quad, Jetson nano, Rasberry Pi, Beagle bone  | Either a yocto/Buildroot ditribution or any embedded or vendor-specific OS (for example Rasbian for Rasberry Pi) |

We also provide a PDF file that has color images of the screenshots/diagrams used in this book. [Click here to download it](https://static.packt-cdn.com/downloads/9781803240060_ColorImages.pdf).

### Related products
* Mastering Linux Device Driver Development [[Packt]](https://www.packtpub.com/product/mastering-linux-device-driver-development/9781789342048?utm_source=github&utm_medium=repository&utm_campaign=9781789342048) [[Amazon]](https://www.amazon.com/dp/1785280007)

* Linux Kernel Programming [[Packt]](https://www.packtpub.com/product/linux-kernel-programming/9781789953435?utm_source=github&utm_medium=repository&utm_campaign=9781789953435) [[Amazon]](https://www.amazon.com/dp/178995343X)

## Get to Know the Author
**John Madieu**
is an embedded Linux and kernel engineer living in Paris, France. His main activity consists of developing device drivers and Board Support Packages (BSPs) for companies in domains including IoT, automation, transport, healthcare, energy, and the military. John is the founder and chief consultant of LABCSMART, a company that provides training and services for embedded Linux and Linux kernel engineering. He is an open source and embedded systems enthusiast, convinced that it is only by sharing knowledge that we can learn more. He is passionate about boxing, which he practiced for 6 years professionally, and continues to channel this passion through training sessions that he provides voluntarily.

## Other books by the author
* [Mastering Linux Device Driver Development](https://www.packtpub.com/product/mastering-linux-device-driver-development/9781789342048?utm_source=github&utm_medium=repository&utm_campaign=9781789342048)
