#include "pch.h"
#include "MultipartPostData.h"
#include "Property.h"
#include "StringUtils.h"
#include <string>
#include <list>
#include <string.h>

net::MultipartPostData::MultipartPostData(const std::string& boundary)
{
    _boundary = boundary;
    _index = 0;
    _state = 0;
    _isFile = false;
    _chunk[0] = 0;
    _chunkSize = sizeof(_chunk);
    _fp = NULL;
    _numWrites = 0;
}

net::MultipartPostData::~MultipartPostData()
{
    if (_fp)
        fclose(_fp);
}

void net::MultipartPostData::processChar(char ch)
{
    if (ch == 13 || _index >= _chunkSize)
    {
        processChunk(_chunk, _index);
        _index = 0;
    }
    _chunk[_index++] = ch;
}

bool net::MultipartPostData::isBoundary(char* chunk, int size)
{
    int boundaryLength = (int)_boundary.length();
    if (size == boundaryLength + 2)
    {
        if (strncmp(&chunk[2], _boundary.c_str(), size - 2) == 0)
            return true;
    }
    else if (size == boundaryLength + 4)
    {
        if (strncmp(&chunk[4], _boundary.c_str(), size - 4) == 0)
            return true;
    }
    else if (size == boundaryLength + 6)
    {
        if (strncmp(&chunk[4], _boundary.c_str(), size - 6) == 0)
            return true;
    }
    return false;
}

void net::MultipartPostData::processChunk(char* chunk, int size)
{
    if (_state == 0) // Waiting for boundary
    {
        if (isBoundary(chunk, size))
        {
            _isFile = false;
            _state = 1;
        }
    }
    else if (_state == 1) // Expecting header line
    {
        if (size == 2)
        {
            _state = 2;
        }
        else
        {
            std::string header(chunk + 2, size - 2);
            std::list<Property> props;
            StringUtils::parseNameValuePairs(props, header.c_str(), ';');
            for (std::list<Property>::iterator it = props.begin(); it != props.end(); ++it)
            {
                if (it->name == "name")
                {
                    _name = it->value;
                }
                else if (it->name == "filename")
                {
                    _isFile = true;
                    UploadedFile uploadedFile;
                    uploadedFile.fileName = it->value;
                    uploadedFile.tempName = SysTempPath() + it->value;
                    files[_name] = uploadedFile;
                    _numWrites = 0;
                    _fp = fopen(uploadedFile.tempName.c_str(), "wb");
                }
            }
        }
    }
    else if (_state == 2) // Expecting data
    {
        if (isBoundary(chunk, size))
        {
            _isFile = false;
            if (_fp)
            {
                fclose(_fp);
                _fp = NULL;
            }
            _state = 1;
        }
        else
        {
            if (_isFile)
            {
                if (_fp)
                {
                    if (_numWrites++ == 0)
                        fwrite(chunk + 2, 1, size - 2, _fp);
                    else
                        fwrite(chunk, 1, size, _fp);
                }
            }
            else if (size > 2)
            {
                std::string value(chunk + 2, size - 2);
                data[_name] = value;
            }
        }
    }
} 
