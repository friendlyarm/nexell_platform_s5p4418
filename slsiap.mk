# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

VENDOR_PATH := vendor/nexell/s5p4418

# ratecontrol
PRODUCT_PACKAGES += \
	libnxvidrc_android

# ogl
PRODUCT_COPY_FILES += \
	$(VENDOR_PATH)/prebuilt/libVR.so:system/lib/libVR.so \
	$(VENDOR_PATH)/prebuilt/libEGL_vr.so:system/lib/egl/libEGL_vr.so \
	$(VENDOR_PATH)/prebuilt/libGLESv1_CM_vr.so:system/lib/egl/libGLESv1_CM_vr.so \
	$(VENDOR_PATH)/prebuilt/libGLESv2_vr.so:system/lib/egl/libGLESv2_vr.so

