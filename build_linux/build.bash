#!/bin/bash

make clean
make timer32=true serial_fifo_enable=true vdev_disable=true enable_bt_serial=true skip_clock_bugfix=true supress_detect_error=true etrobo_optimize=true reuse_port=true
