#ifndef CA4G_H
#define CA4G_H

#include "ca4g_dxr_support.h"
#include "ca4g_scene.h"

#pragma region DSL commands

#define _set ->set->
#define _clear ->clear->
#define _load ->load->
#define _create ->create->
#define _dispatch ->dispatch->
#define _copy ->copy->

#define __set this _set
#define __clear this _clear
#define __load this _load
#define __create this _create
#define __dispatch this _dispatch

#define render_target this->GetRenderTarget()

template<typename T>
T dr(T* a) { return &a; }

#define member_collector(methodName) Collector(this, &decltype(dr(this))::methodName)
#define member_collector_async(methodName) Collector_Async(this, &decltype(dr(this))::methodName)

#pragma endregion


#endif