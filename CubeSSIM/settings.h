#pragma once

enum EImageFormat { MONOCHROME, RGBA, YCbCrA };

const EImageFormat g_ImageFormat = YCbCrA;

const int g_SSIMKernelSize = 11;
const float g_SSIMSigma = 1.5f;

const float g_IMEDSigma = 10.0f;

// SSIM settings
const float g_Alpha = 1.0f;  // Luminance weight
const float g_Beta = 1.0f;   // Contrast weight
const float g_Gamma = 1.0f;  // Structure weight
const float g_L = 1;         // 255?
const float g_K1 = 0.001f;
const float g_K2 = 0.003f;
const float g_C1 = (g_K1 * g_L) * (g_K1 * g_L);
const float g_C2 = (g_K2 * g_L) * (g_K2 * g_L);
const float g_C3 = g_C2 / 2.0f;

const int g_FaceCount = 6;
const std::string g_FaceNames[g_FaceCount] = { "XP", "XN", "YP", "YN", "ZP", "ZN" };