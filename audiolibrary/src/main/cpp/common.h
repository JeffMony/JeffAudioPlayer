//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_common_h
#define jeff_common_h

#define SHORT_SIZE sizeof(short)
#define UINT8_SIZE sizeof(uint8_t)

#define RESAMPLE_CHANNEL 2
#define RESAMPLE_RATE 44100
#define AUDIO_UNIT_LENGTH 1024
#define AUDIO_BUFFER_LENGTH (1024 * 16)

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) { if (x) delete (x); (x) = nullptr;}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) { if (x) delete [] (x); (x) = nullptr;}
#endif

#ifndef STRUCT_ARRAY_ELEMS
#define STRUCT_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#endif

#endif //jeff_common_h
