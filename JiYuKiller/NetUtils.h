#pragma once
#include "stdafx.h"
#include <string>
#include <curl/curl.h>

using namespace std;

CURLcode curl_get_req(const string &url, string &response);
CURLcode curl_post_req(const string &url, const string &postParams, string &response);
