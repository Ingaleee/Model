#pragma once

#if COUPLING_USE_KOMPAS_SDK
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <ksConstants.h>
#include <ksConstants3D.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef GetObject
#undef GetObject
#endif
#import "kAPI5.tlb" rename_namespace("KompasAPI") named_guids
#endif
