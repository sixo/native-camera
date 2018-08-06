// Copyright (c) 2018 Roman Sisik

#ifndef NATIVE_CAMERA_CAM_UTILS_H
#define NATIVE_CAMERA_CAM_UTILS_H

#include <camera/NdkCameraManager.h>
#include <string>

namespace sixo {

void printCamProps(ACameraManager *cameraManager, const char *id);

std::string getBackFacingCamId(ACameraManager *cameraManager);

}
#endif //NATIVE_CAMERA_CAM_UTILS_H
