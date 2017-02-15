//
//  WideString.cpp
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#include "WideString.h"

WideString::WideString(const char* str) : v(strlen(str)+1) {
	std::copy(str, str + v.size(), v.begin());
}

WideString::WideString(const std::string& str) : v(str.size()+1) {
	std::copy(str.begin(), str.end(), v.begin());
	*v.rbegin() = 0;
}

WideString::operator wchar_t* () {
	return &v[0];
}
