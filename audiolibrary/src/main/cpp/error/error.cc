//
// Created by jeffli on 2021/9/7.
//

#include "error.h"

namespace media {

int Error::UNKNOWN_ERROR = -100;

static const Error::Info infos[] = {

};

const Error::Info * Error::GetErrorInfo(int code) {
    const struct Error::Info *info = nullptr;
    for (int i = 0; i < STRUCT_ARRAY_ELEMS(infos); i++) {
        if (code == infos[i].code) {
            info = &infos[i];
            break;
        }
    }
    if (info == nullptr) {
        info = new Error::Info{ Error::UNKNOWN_ERROR, "unknown error" };
    }
    return info;
}

}
