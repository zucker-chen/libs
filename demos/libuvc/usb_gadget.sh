#!/bin/sh


USB_VID=0x12d1
USB_PID=0x4321
USB_GADGET_NAME=g1
USB_ATTRIBUTE=0x409
USB_SKELETON=c.1	# b.1
CONFIGFS_DIR=/sys/kernel/config
USB_CONFIGFS_DIR=${CONFIGFS_DIR}/usb_gadget/${USB_GADGET_NAME}
USB_STRINGS_DIR=${USB_CONFIGFS_DIR}/strings/${USB_ATTRIBUTE}
USB_FUNCTIONS_DIR=${USB_CONFIGFS_DIR}/functions
USB_CONFIGS_DIR=${USB_CONFIGFS_DIR}/configs/${USB_SKELETON}

UVC_DIR=${USB_FUNCTIONS_DIR}/uvc.usb0	# gs6
UVC_STREAMING_DIR=${UVC_DIR}/streaming
UVC_CONTROL_DIR=${UVC_DIR}/control
UVC_U_DIR=${UVC_STREAMING_DIR}/uncompressed/u
UVC_M_DIR=${UVC_STREAMING_DIR}/mjpeg/m
UVC_F_DIR=${UVC_STREAMING_DIR}/framebased/f
UVC_F2_DIR=${UVC_STREAMING_DIR}/framebased/f2



configfs_init()
{
	echo "Debug: configfs_init"

	mount -t configfs none ${CONFIGFS_DIR}
	mkdir ${USB_CONFIGFS_DIR} -m 0770
	echo $USB_VID > ${USB_CONFIGFS_DIR}/idVendor
	echo $USB_PID > ${USB_CONFIGFS_DIR}/idProduct
	echo 0x0310 > ${USB_CONFIGFS_DIR}/bcdDevice
	echo 0x0200 > ${USB_CONFIGFS_DIR}/bcdUSB
	mkdir ${USB_STRINGS_DIR}   -m 0770
	SERIAL=0123456789ABCDEF
	echo $SERIAL > ${USB_STRINGS_DIR}/serialnumber
	echo "czqiang"  > ${USB_STRINGS_DIR}/manufacturer
	echo "ev200"  > ${USB_STRINGS_DIR}/product

	mkdir ${USB_CONFIGS_DIR}  -m 0770
	mkdir ${USB_CONFIGS_DIR}/strings/${USB_ATTRIBUTE}  -m 0770
	echo 500 > ${USB_CONFIGS_DIR}/MaxPower
	echo 0xc0 > ${USB_CONFIGS_DIR}/bmAttributes

	#echo 0x1 > ${USB_CONFIGFS_DIR}/os_desc/b_vendor_code
	#echo "MSFT100" > ${USB_CONFIGFS_DIR}/os_desc/qw_sign
	#ln -s ${USB_CONFIGS_DIR} ${USB_CONFIGFS_DIR}/os_desc/b.1
}

configure_uvc_resolution_yuyv()
{
    UVC_DISPLAY_W=$1
    UVC_DISPLAY_H=$2
    mkdir ${UVC_U_DIR}/${UVC_DISPLAY_H}p
    echo $UVC_DISPLAY_W > ${UVC_U_DIR}/${UVC_DISPLAY_H}p/wWidth
    echo $UVC_DISPLAY_H > ${UVC_U_DIR}/${UVC_DISPLAY_H}p/wHeight
    echo 333333 > ${UVC_U_DIR}/${UVC_DISPLAY_H}p/dwDefaultFrameInterval
    echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*20)) > ${UVC_U_DIR}/${UVC_DISPLAY_H}p/dwMinBitRate
    echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*20)) > ${UVC_U_DIR}/${UVC_DISPLAY_H}p/dwMaxBitRate
    echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*2)) > ${UVC_U_DIR}/${UVC_DISPLAY_H}p/dwMaxVideoFrameBufferSize
    echo -e "333333\n666666\n1000000\n2000000" > ${UVC_U_DIR}/${UVC_DISPLAY_H}p/dwFrameInterval
}

configure_uvc_resolution_mjpeg()
{
    UVC_DISPLAY_W=$1
    UVC_DISPLAY_H=$2
    mkdir ${UVC_M_DIR}/${UVC_DISPLAY_H}p
    echo $UVC_DISPLAY_W > ${UVC_M_DIR}/${UVC_DISPLAY_H}p/wWidth
    echo $UVC_DISPLAY_H > ${UVC_M_DIR}/${UVC_DISPLAY_H}p/wHeight
    echo 333333 > ${UVC_M_DIR}/${UVC_DISPLAY_H}p/dwDefaultFrameInterval
    echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*20)) > ${UVC_M_DIR}/${UVC_DISPLAY_H}p/dwMinBitRate
    echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*20)) > ${UVC_M_DIR}/${UVC_DISPLAY_H}p/dwMaxBitRate
    echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*2)) > ${UVC_M_DIR}/${UVC_DISPLAY_H}p/dwMaxVideoFrameBufferSize
    echo -e "333333\n666666\n1000000\n2000000" > ${UVC_M_DIR}/${UVC_DISPLAY_H}p/dwFrameInterval
}
configure_uvc_resolution_h264()
{
    UVC_DISPLAY_W=$1
    UVC_DISPLAY_H=$2
	mkdir ${UVC_F_DIR}/${UVC_DISPLAY_H}p
	echo $UVC_DISPLAY_W > ${UVC_F_DIR}/${UVC_DISPLAY_H}p/wWidth
	echo $UVC_DISPLAY_H > ${UVC_F_DIR}/${UVC_DISPLAY_H}p/wHeight
	echo 333333 > ${UVC_F_DIR}/${UVC_DISPLAY_H}p/dwDefaultFrameInterval
	echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*10)) > ${UVC_F_DIR}/${UVC_DISPLAY_H}p/dwMinBitRate
	echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*10)) > ${UVC_F_DIR}/${UVC_DISPLAY_H}p/dwMaxBitRate
	echo -e "333333\n400000\n500000\n666666\n1000000\n2000000" > ${UVC_F_DIR}/${UVC_DISPLAY_H}p/dwFrameInterval
}
configure_uvc_resolution_h265()
{
	UVC_DISPLAY_W=$1
	UVC_DISPLAY_H=$2
	mkdir ${UVC_F2_DIR}/${UVC_DISPLAY_H}p
	echo $UVC_DISPLAY_W > ${UVC_F2_DIR}/${UVC_DISPLAY_H}p/wWidth
	echo $UVC_DISPLAY_H > ${UVC_F2_DIR}/${UVC_DISPLAY_H}p/wHeight
	echo 333333 > ${UVC_F2_DIR}/${UVC_DISPLAY_H}p/dwDefaultFrameInterval
	echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*10)) > ${UVC_F2_DIR}/${UVC_DISPLAY_H}p/dwMinBitRate
	echo $((UVC_DISPLAY_W*UVC_DISPLAY_H*10)) > ${UVC_F2_DIR}/${UVC_DISPLAY_H}p/dwMaxBitRate
	echo -e "333333\n400000\n500000" > ${UVC_F2_DIR}/${UVC_DISPLAY_H}p/dwFrameInterval
}
uvc_device_config()
{
	mkdir ${UVC_DIR}
	#echo 3072 > ${UVC_STREAMING_DIR}_maxpacket
	#echo 2 > ${UVC_DIR}/uvc_num_request
	#echo 1 > ${UVC_DIR}/streaming_bulk
	
	mkdir ${UVC_CONTROL_DIR}/header/h
	echo "0x0110" > ${UVC_CONTROL_DIR}/header/h/bcdUVC
	echo "48000000" > ${UVC_CONTROL_DIR}/header/h/dwClockFrequency
	ln -s ${UVC_CONTROL_DIR}/header/h ${UVC_CONTROL_DIR}/class/fs/h
	ln -s ${UVC_CONTROL_DIR}/header/h ${UVC_CONTROL_DIR}/class/ss/h

	#echo -e "0xa\n0x0\n0x0" > ${UVC_CONTROL_DIR}/terminal/camera/default/bmControls
	#echo -e "0x4f\n0x14" > ${UVC_CONTROL_DIR}/processing/default/bmControls
	
	##YUYV support config
	mkdir ${UVC_U_DIR}
	#configure_uvc_resolution_yuyv 1280 720
	#configure_uvc_resolution_yuyv 640 480
	#configure_uvc_resolution_yuyv 320 240
	#echo "1" > ${UVC_U_DIR}/bDefaultFrameIndex

	##mjpeg support config
	mkdir ${UVC_M_DIR}
	configure_uvc_resolution_mjpeg 1920 1080
	configure_uvc_resolution_mjpeg 1280 720
	configure_uvc_resolution_mjpeg 640 480
	## default mjpeg 1280*720
	echo "1" > ${UVC_M_DIR}/bDefaultFrameIndex

	## h.264 support config
	mkdir ${UVC_F_DIR}
	#configure_uvc_resolution_h264 2560 1440
	#configure_uvc_resolution_h264 3840 2160
	configure_uvc_resolution_h264 1920 1080
	configure_uvc_resolution_h264 1280 720
	configure_uvc_resolution_h264 640 480
	echo "1" > ${UVC_F_DIR}/bDefaultFrameIndex

	## h.265 support config
	mkdir ${UVC_F2_DIR}
	#configure_uvc_resolution_h265 3840 2160
	#configure_uvc_resolution_h265 2560 1440
	#configure_uvc_resolution_h265 1920 1080
	#configure_uvc_resolution_h265 1280 720
	#configure_uvc_resolution_h265 640 480
	#echo "1" > ${UVC_F2_DIR}/bDefaultFrameIndex

	mkdir ${UVC_STREAMING_DIR}/header/h
	#ln -s ${UVC_U_DIR} ${UVC_STREAMING_DIR}/header/h/u
	ln -s ${UVC_M_DIR} ${UVC_STREAMING_DIR}/header/h/m
	ln -s ${UVC_F_DIR} ${UVC_STREAMING_DIR}/header/h/f1
	#ln -s ${UVC_F2_DIR} ${UVC_STREAMING_DIR}/header/h/f2
	ln -s ${UVC_STREAMING_DIR}/header/h ${UVC_STREAMING_DIR}/class/fs/h
	ln -s ${UVC_STREAMING_DIR}/header/h ${UVC_STREAMING_DIR}/class/hs/h
	ln -s ${UVC_STREAMING_DIR}/header/h ${UVC_STREAMING_DIR}/class/ss/h
}
uac_device_config()
{
  UAC=$1
  mkdir ${USB_FUNCTIONS_DIR}/${UAC}.gs0
  UAC_GS0=${USB_FUNCTIONS_DIR}/${UAC}.gs0
  echo 3 > ${UAC_GS0}/p_chmask
  echo 2 > ${UAC_GS0}/p_ssize
  echo 8000,16000,44100,48000 > ${UAC_GS0}/p_srate

  echo 3 > ${UAC_GS0}/c_chmask
  echo 2 > ${UAC_GS0}/p_ssize
  echo 8000,16000,44100,48000 > ${UAC_GS0}/c_srate

  ln -s ${UAC_GS0} ${USB_CONFIGS_DIR}/f2
}
pre_run_rndis()
{
  RNDIS_STR="rndis"
  if ( echo $1 |grep -q "rndis" ); then
   #sleep 1
   IP_FILE=/data/uvc_xu_ip_save
   echo "config usb0 IP..."
   if [ -f $IP_FILE ]; then
      for line in `cat $IP_FILE`
      do
        echo "save ip is: $line"
        ifconfig usb0 $line
      done
   else
    ifconfig usb0 172.16.110.6
   fi
   ifconfig usb0 up
  fi
}



##main
#init usb config
configfs_init

echo 0x0020 > ${USB_CONFIGFS_DIR}/idProduct

#uvc config init
uvc_device_config

##reset config,del default adb config
if [ -e ${USB_CONFIGS_DIR}/ffs.adb ]; then
   #for rk1808 kernel 4.4
   rm -f ${USB_CONFIGS_DIR}/ffs.adb
else
   ls ${USB_CONFIGS_DIR} | grep f[0-9] | xargs -I {} rm ${USB_CONFIGS_DIR}/{}
fi

case "$1" in
rndis)
    # config rndis
   mkdir ${USB_CONFIGFS_DIR}/functions/rndis.gs0
   echo "uvc_rndis" > ${USB_STRINGS_DIR}/configuration
   ln -s ${USB_FUNCTIONS_DIR}/rndis.gs0 ${USB_CONFIGS_DIR}/f2
   echo "config uvc and rndis..."
   ;;
uac1)
   uac_device_config uac1
   echo "uvc_uac1" > ${USB_STRINGS_DIR}/configuration
   echo "config uvc and uac1..."
   ;;
uac2)
   uac_device_config uac2
   echo "uvc_uac2" > ${USB_STRINGS_DIR}/configuration
   echo "config uvc and uac2..."
   ;;
uac1_rndis)
   uac_device_config uac1
   mkdir ${USB_CONFIGFS_DIR}/functions/rndis.gs0
   ln -s ${USB_FUNCTIONS_DIR}/rndis.gs0 ${USB_CONFIGS_DIR}/f3
   echo "uvc_uac1_rndis" > ${USB_STRINGS_DIR}/configuration
   echo "config uvc and uac1 rndis..."
   ;;
uac2_rndis)
   uac_device_config uac2
   mkdir ${USB_CONFIGFS_DIR}/functions/rndis.gs0
   ln -s ${USB_FUNCTIONS_DIR}/rndis.gs0 ${USB_CONFIGS_DIR}/f3
   echo "uvc_uac2_rndis" > ${USB_STRINGS_DIR}/configuration
   echo "config uvc and uac2 rndis..."
   ;;
*)
   echo "Config 1" > ${USB_STRINGS_DIR}/configuration
   echo "config uvc ..."
esac

ln -s ${UVC_DIR} ${USB_CONFIGS_DIR}

UDC=`ls /sys/class/udc/| awk '{print $1}'`
echo $UDC > ${USB_CONFIGFS_DIR}/UDC

echo "0x01" > ${USB_CONFIGFS_DIR}/bDeviceProtocol
echo "0x02" > ${USB_CONFIGFS_DIR}/bDeviceSubClass
echo "0xEF" > ${USB_CONFIGFS_DIR}/bDeviceClass


#if [ "$1" ]; then
#  pre_run_rndis $1
#fi
