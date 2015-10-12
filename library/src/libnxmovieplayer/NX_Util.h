#ifndef __NX_Util_H__
#define __NX_Util_H__

#ifdef __cplusplus
extern "C" {
#endif	//	__cplusplus
long long NX_GetTime();
void SWVolume(int request_volume,int sample_size, short *pcm_sample );  //request_volume : range : 1~100


#ifdef __cplusplus
}
#endif


#endif // __NX_Util_H__
