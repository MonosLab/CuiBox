// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

typedef void(*CALLBACK_FUNC)(bool);				// 함수 포인터 정의
extern CALLBACK_FUNC cbf;						// 실제 콜백함수를 다루기 위한 전역 변수

static void RegistCallback(CALLBACK_FUNC cb)	// 콜백 등록 함수
{
	cbf = cb;
}

#endif //PCH_H
