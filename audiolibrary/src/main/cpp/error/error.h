//
// Created by jeffli on 2021/9/7.
//

#ifndef jeff_error_h
#define jeff_error_h

#include "common.h"

namespace media {

class Error {

public:

    struct Info {
        int code;
        const char* msg;
    };

    static const Error::Info* GetErrorInfo(int code);


public:

    static int UNKNOWN_ERROR;
};

}


#endif //jeff_error_h
