#pragma once

#include "image.h"
#include "settings.h"

#include <opencv2/opencv.hpp>

#include <filesystem>
#include <string>

using ScoreType = cv::Vec3f;

int DivUp(int TotalShaderCount, int WorkGroupSize);

int GetKernelSize(float Sigma, float MinWeight);

float GetGaussian(float D, float Sigma);
float GetGaussian(int u, int v, float Sigma);

bool IsExtensionAvailable(const std::string& Extension);

GLuint LoadShader(const std::initializer_list<std::string>& Paths);

SImage LoadImage(const std::string& Path);
SImage LoadCubemap(const std::string& Path);