## ISDN3000e-Lab8

This is the lab of the course **ISDN3000e: Programming for Integrative Systems** at HKUST led by Prof. Ziqi Wang. 

The lab requires **libopencv**, **librealsense** for image processing and camera input source. Please install them following the commands below.

### Linux / WSL2 (Ubuntu 20.04 / 22.04 / 24.04)

Run the following commands in your **Ubuntu / WSL2 terminal**:

#### Download OpenCV

```bash
set -e

cd ~

sudo apt update
sudo apt install -y \
  g++ cmake make wget unzip pkg-config \
  libgtk-3-dev \
  libjpeg-dev libpng-dev libtiff-dev libopenjp2-7-dev \
  libavcodec-dev libavformat-dev libswscale-dev

wget -O opencv.zip https://github.com/opencv/opencv/archive/4.9.0.zip
wget -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/4.9.0.zip

unzip opencv.zip
unzip opencv_contrib.zip

mv opencv-4.9.0 opencv
mv opencv_contrib-4.9.0 opencv_contrib

cd ~/opencv
mkdir -p build
cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DOPENCV_EXTRA_MODULES_PATH=~/opencv_contrib/modules \
  -DBUILD_LIST=core,imgproc,imgcodecs,highgui,videoio,dnn,calib3d,aruco \
  -DBUILD_TESTS=OFF \
  -DBUILD_PERF_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF

make -j"$(nproc)"
sudo make install
sudo ldconfig

pkg-config --modversion opencv4
```

#### Download librealsense
```bash
cd ~

sudo apt update
sudo apt install -y libudev-dev libusb-1.0-0-dev

git clone https://github.com/IntelRealSense/librealsense.git
cd librealsense
mkdir build
cd build
cmake .. -DBUILD_EXAMPLES=false -DBUILD_GRAPHICAL_EXAMPLES=false
make -j$(nproc)
sudo make install
```

#### Set up your camera devices in WSL2
Install the usbipd:
```powershell
winget install usbipd
```
Check the connected devices:
```powershell
usbipd list
```
Attach the target devices to WSL2 to make a USB camera visible inside WSL2:
```powershell
usbipd bind --busid 4-4
usbipd attach --wsl --busid 4-4
```
After attaching, the device will be available to WSL2.


### MacOS (Homebrew)

Run the following command in your terminal:

```bash
brew install opencv
brew install librealsense
```
