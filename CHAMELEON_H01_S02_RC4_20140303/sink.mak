
#Check the defines are valid for the hardware
ifneq (, $(findstring -DBC5_MULTIMEDIA,$(DEFS)))      
    
    ifneq (, $(findstring -DENABLE_GATT,$(DEFS)))  
        $(error GATT is not supported on BlueCore5-MultiMedia)
    endif
    
    ifneq (, $(findstring -DENABLE_CAPSENSE,$(DEFS)))  
        $(error Capsense is not supported on BlueCore5-MultiMedia)
    endif
    
    ifneq (, $(findstring -DENABLE_REMOTE,$(DEFS)))  
        $(error Ble Remote is not supported on BlueCore5-MultiMedia)
    endif
    
endif

# ensure we always run the rules for each kalimba app to get the latest version
.PHONY : image/START.HTM \
	 image/usb_root \
	 image/usb_fat \
     image/cvc_headset/cvc_headset.kap \
	 image/cvc_headset_bex/cvc_headset_bex.kap \
	 image/cvc_headset_wb_wbs/cvc_headset_wb_wbs.kap \
	 image/cvc_headset_2mic/cvc_headset_2mic.kap \
	 image/cvc_headset_2mic_bex/cvc_headset_2mic_bex.kap \
	 image/cvc_headset_2mic_wb_wbs/cvc_headset_2mic_wb_wbs.kap \
     image/cvc_handsfree/cvc_handsfree.kap \
	 image/cvc_handsfree_bex/cvc_handsfree_bex.kap \
	 image/cvc_handsfree_wb_wbs/cvc_handsfree_wb_wbs.kap \
	 image/sbc_decoder/sbc_decoder.kap \
     image/aptx_decoder/aptx_decoder.kap \
	 image/aptx_acl_sprint_decoder/aptx_acl_sprint_decoder.kap \
	 image/faststream_decoder/faststream_decoder.kap \
     image/mp3_decoder/mp3_decoder.kap \
     image/aac_decoder/aac_decoder.kap
     
#  1 mic CVC
image/cvc_headset/cvc_headset.kap :
	$(mkdir) image/cvc_headset
	$(copyfile) ..\..\kalimba\apps\cvc_headset\image\cvc_headset\cvc_headset.kap $@

image.fs : image/cvc_headset/cvc_headset.kap

#  1 mic CVC BEX
image/cvc_headset_bex/cvc_headset_bex.kap :
	$(mkdir) image/cvc_headset_bex
	$(copyfile) ..\..\kalimba\apps\cvc_headset\image\cvc_headset_bex\cvc_headset_bex.kap $@

image.fs : image/cvc_headset_bex/cvc_headset_bex.kap

#  1 mic CVC WBS
image/cvc_headset_wb_wbs/cvc_headset_wb_wbs.kap :
	$(mkdir) image/cvc_headset_wb_wbs
	$(copyfile) ..\..\kalimba\apps\cvc_headset\image\cvc_headset_wb_wbs\cvc_headset_wb_wbs.kap $@

image.fs : image/cvc_headset_wb_wbs/cvc_headset_wb_wbs.kap

# 2 mic CVC
image/cvc_headset_2mic/cvc_headset_2mic.kap :
	$(mkdir) image/cvc_headset_2mic
	$(copyfile) ..\..\kalimba\apps\cvc_headset_2mic\image\cvc_headset_2mic\cvc_headset_2mic.kap $@

image.fs : image/cvc_headset_2mic/cvc_headset_2mic.kap

# 2 mic CVC BEX
image/cvc_headset_2mic_bex/cvc_headset_2mic_bex.kap :
	$(mkdir) image/cvc_headset_2mic_bex
	$(copyfile) ..\..\kalimba\apps\cvc_headset_2mic\image\cvc_headset_2mic_bex\cvc_headset_2mic_bex.kap $@

image.fs : image/cvc_headset_2mic_bex/cvc_headset_2mic_bex.kap

# 2 mic CVC WBS
image/cvc_headset_2mic_wb_wbs/cvc_headset_2mic_wb_wbs.kap :
	$(mkdir) image/cvc_headset_2mic_wb_wbs
	$(copyfile) ..\..\kalimba\apps\cvc_headset_2mic\image\cvc_headset_2mic_wb_wbs\cvc_headset_2mic_wb_wbs.kap $@

image.fs : image/cvc_headset_2mic_wb_wbs/cvc_headset_2mic_wb_wbs.kap

#  1 mic Handsfree CVC
image/cvc_handsfree/cvc_handsfree.kap :
	$(mkdir) image/cvc_handsfree
	$(copyfile) ..\..\kalimba\apps\cvc_handsfree\image\cvc_handsfree\cvc_handsfree.kap $@

image.fs : image/cvc_handsfree/cvc_handsfree.kap

#  1 mic Handsfree CVC BEX
image/cvc_handsfree_bex/cvc_handsfree_bex.kap :
	$(mkdir) image/cvc_handsfree_bex
	$(copyfile) ..\..\kalimba\apps\cvc_handsfree\image\cvc_handsfree_bex\cvc_handsfree_bex.kap $@

image.fs : image/cvc_handsfree_bex/cvc_handsfree_bex.kap

#  1 mic Handsfree CVC WBS
image/cvc_handsfree_wb_wbs/cvc_handsfree_wb_wbs.kap :
	$(mkdir) image/cvc_handsfree_wb_wbs
	$(copyfile) ..\..\kalimba\apps\cvc_handsfree\image\cvc_handsfree_wb_wbs\cvc_handsfree_wb_wbs.kap $@

image.fs : image/cvc_handsfree_wb_wbs/cvc_handsfree_wb_wbs.kap

######################################################################################################
##### A2DP DECODER VERSIONS
######################################################################################################

# copy in sbc decoder 
image/sbc_decoder/sbc_decoder.kap :
	$(mkdir) image/sbc_decoder
	$(copyfile) ..\..\kalimba\apps\a2dp_sink\image\sbc_decoder\sbc_decoder.kap $@

image.fs : image/sbc_decoder/sbc_decoder.kap

# copy in faststream
image/faststream_decoder/faststream_decoder.kap :
	$(mkdir) image/faststream_decoder
	$(copyfile) ..\..\kalimba\apps\a2dp_sink\image\faststream_decoder\faststream_decoder.kap $@

image.fs : image/faststream_decoder/faststream_decoder.kap

# copy in mp3 decoder 
#image/mp3_decoder/mp3_decoder.kap :
#	$(mkdir) image/mp3_decoder
#	$(copyfile) ..\..\kalimba\apps\a2dp_sink\image\mp3_decoder\mp3_decoder.kap $@

#image.fs : image/mp3_decoder/mp3_decoder.kap

# copy in aac decoder 
#image/aac_decoder/aac_decoder.kap :
#	$(mkdir) image/aac_decoder
#	$(copyfile) ..\..\kalimba\apps\a2dp_sink\image\aac_decoder\aac_decoder.kap $@

#image.fs : image/aac_decoder/aac_decoder.kap

# copy in apt-X decoder
#image/aptx_decoder/aptx_decoder.kap :
#	$(mkdir) image/aptx_decoder
#	$(copyfile) ..\..\kalimba\apps\a2dp_sink\image\aptx_decoder\aptx_decoder.kap $@

#image.fs : image/aptx_decoder/aptx_decoder.kap

# copy in apt-X ACL Sprint decoder
#image/aptx_acl_sprint_decoder/aptx_acl_sprint_decoder.kap :
#	$(mkdir) image/aptx_acl_sprint_decoder
#	$(copyfile) ..\..\kalimba\apps\a2dp_sink\image\aptx_acl_sprint_decoder\aptx_acl_sprint_decoder.kap $@

#image.fs : image/aptx_acl_sprint_decoder/aptx_acl_sprint_decoder.kap



######################################################################################################
##### USB FILES
######################################################################################################

# If COPY_USB_MS_README is defined
ifneq (,$(findstring -DCOPY_USB_MS_README,$(DEFS)))

# Copy START.HTM into image folder
image/START.HTM:
	$(copyfile) START.HTM $@
image.fs : image/START.HTM

# Copy USB Root info to image folder
image/usb_root:
	$(copyfile) usb_root $@
image.fs : image/usb_root

# Copy USB FAT info to image folder
image/usb_fat:
	$(copyfile) usb_fat $@
image.fs : image/usb_fat


# If COPY_USB_MS_README not defined
else

# Remove START.HTM and USB mass storage info
image.fs : | remove_usb_ms

remove_usb_ms :
	$(del) image/START.HTM
	$(del) image/usb_root
	$(del) image/usb_fat

endif
