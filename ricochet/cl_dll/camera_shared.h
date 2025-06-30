// camera_shared.h

#ifndef CAMERA_SHARED_H
#define CAMERA_SHARED_H

#ifdef __cplusplus
extern "C" {
#endif

	extern float vecNewViewAngles[3];
	extern float vecNewViewOrigin[3];
	extern int iHasNewViewAngles;
	extern int iHasNewViewOrigin;
	extern int iIsSpectator;

#ifdef __cplusplus
}
#endif

#endif
