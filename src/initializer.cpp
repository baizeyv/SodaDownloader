//
// Created by baizeyv on 2/1/2026.
//
#include "../include/initializer.h"

#include <curl/curl.h>

void initializer::setup()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

void initializer::dispose()
{
    curl_global_cleanup();
}
