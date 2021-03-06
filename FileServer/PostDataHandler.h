#pragma once

#include "UploadedFile.h"
#include <string>

namespace net
{
    class PostDataHandler
    {
    public:
        virtual void addDataPair(std::string_view name, std::string_view value) = 0;
        virtual void addUploadedFile(const UploadedFile& uploadedFile) = 0;
    };
}
